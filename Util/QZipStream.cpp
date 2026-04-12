#include "QZipStream.h"

#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <zlib.h>

#include "Util.h"

// Zip文件头结构
#pragma pack(push, 1)
struct LocalFileHeader {
    quint32 signature;           // 0x04034b50
    quint16 versionNeeded;       // 20
    quint16 flags;               // 0
    quint16 compressionMethod;   // 8 = DEFLATE, 0 = STORE
    quint16 lastModTime;         // DOS时间
    quint16 lastModDate;         // DOS日期
    quint32 crc32;               // CRC32校验
    quint32 compressedSize;      // 压缩后大小
    quint32 uncompressedSize;    // 原始大小
    quint16 fileNameLength;      // 文件名长度
    quint16 extraFieldLength;    // 额外字段长度
};

struct CentralDirHeader {
    quint32 signature;           // 0x02014b50
    quint16 versionMadeBy;       // 20
    quint16 versionNeeded;       // 20
    quint16 flags;               // 0
    quint16 compressionMethod;   // 8 = DEFLATE, 0 = STORE
    quint16 lastModTime;         // DOS时间
    quint16 lastModDate;         // DOS日期
    quint32 crc32;               // CRC32校验
    quint32 compressedSize;      // 压缩后大小
    quint32 uncompressedSize;    // 原始大小
    quint16 fileNameLength;      // 文件名长度
    quint16 extraFieldLength;    // 额外字段长度
    quint16 fileCommentLength;   // 注释长度
    quint16 diskNumberStart;     // 磁盘号起始
    quint16 internalFileAttr;    // 内部属性
    quint32 externalFileAttr;    // 外部属性
    quint32 localHeaderOffset;   // 本地头偏移
};

struct EndOfCentralDir {
    quint32 signature;                 // 0x06054b50
    quint16 diskNumber;                // 当前磁盘号
    quint16 diskWithCentralDir;        // 中央目录磁盘号
    quint16 centralDirEntriesOnDisk;   // 本磁盘中央目录条目数
    quint16 totalCentralDirEntries;    // 总中央目录条目数
    quint32 centralDirSize;            // 中央目录大小
    quint32 centralDirOffset;          // 中央目录偏移
    quint16 commentLength;             // 注释长度
};
#pragma pack(pop)

QZipStream::QZipStream(QObject* parent) : QObject(parent), m_compressionLevel(Default)
{
}

QZipStream::~QZipStream()
{
}

bool QZipStream::startCompress(const QString& outFile)
{
    if (QFile::exists(outFile)) {
        m_lastError = QString("输出文件已存在: %1").arg(outFile);
        return false;
    }

    m_outputFileName = outFile;
    m_files.clear();
    m_centralRecords.clear();
    m_currentProgress = 0;
    m_totalProgress = 0;
    m_lastError.clear();

    emit compressionStarted();
    return true;
}

bool QZipStream::addFolder(const QString& dir)
{
    QDir directory(dir);
    if (!directory.exists()) {
        m_lastError = QString("文件夹不存在: %1").arg(dir);
        emit errorOccurred(m_lastError);
        return false;
    }

    QString basePath = QFileInfo(dir).absolutePath();
    if (!basePath.endsWith('/') && !basePath.endsWith('\\')) {
        basePath += '/';
    }

    // 递归添加文件夹中的所有文件
    QFileInfoList allFiles =
        directory.entryInfoList(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Hidden, QDir::Name);

    m_totalProgress += allFiles.size();

    foreach (const QFileInfo& fileInfo, allFiles) {
        m_currentFile = fileInfo.fileName();
        m_currentProgress++;

        emit progressChanged(m_currentProgress, m_totalProgress, m_currentFile);

        if (!addFileInternal(fileInfo.absoluteFilePath(), fileInfo.fileName())) {
            return false;
        }

        emit fileAdded(fileInfo.absoluteFilePath(), fileInfo.size());
    }

    // 递归处理子文件夹
    QFileInfoList subdirs = directory.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::Name);

    foreach (const QFileInfo& dirInfo, subdirs) {
        QString subdirPath = dirInfo.absoluteFilePath();
        QString subdirName = dirInfo.fileName();

        // 递归添加子文件夹
        if (!addFolder(subdirPath)) {
            return false;
        }
    }

    return true;
}

bool QZipStream::addFile(const QString& filePath)
{
    m_currentFile = QFileInfo(filePath).fileName();
    m_currentProgress++;
    m_totalProgress++;

    emit progressChanged(m_currentProgress, m_totalProgress, m_currentFile);
    emit fileAdded(filePath, QFileInfo(filePath).size());

    return addFileInternal(filePath, QFileInfo(filePath).fileName());
}

bool QZipStream::endCompress()
{
    if (m_outputFileName.isEmpty()) {
        m_lastError = "未设置输出文件";
        emit errorOccurred(m_lastError);
        return false;
    }

    if (m_files.isEmpty()) {
        m_lastError = "没有要压缩的文件";
        emit errorOccurred(m_lastError);
        return false;
    }

    QFile zipFile(m_outputFileName);
    if (!zipFile.open(QIODevice::WriteOnly)) {
        m_lastError = QString("无法创建zip文件: %1").arg(m_outputFileName);
        emit errorOccurred(m_lastError);
        return false;
    }

    QDataStream out(&zipFile);
    out.setByteOrder(QDataStream::LittleEndian);   // Zip使用小端序

    m_centralRecords.clear();

    // 写入所有文件的本地头和数据
    for (int i = 0; i < m_files.size(); i++) {
        const FileInfo& fileInfo = m_files[i];

        // 压缩数据
        QByteArray compressedData;
        if (m_compressionLevel != NoCompression) {
            if (!compressDataDeflate(fileInfo.data, compressedData, m_compressionLevel)) {   // 改为deflate
                m_lastError = QString("压缩文件失败: %1").arg(fileInfo.relativePath);
                emit errorOccurred(m_lastError);
                zipFile.close();
                zipFile.remove();
                return false;
            }
        } else {
            compressedData = fileInfo.data;   // 不压缩
        }

        // 写入本地文件头
        LocalFileHeader localHeader;
        localHeader.signature = 0x04034b50;
        localHeader.versionNeeded = 20;
        localHeader.flags = 0;
        localHeader.compressionMethod = (m_compressionLevel == NoCompression) ? 0 : 8;   // 0=STORE, 8=DEFLATE
        localHeader.lastModTime = fileInfo.dosTime;
        localHeader.lastModDate = fileInfo.dosDate;
        localHeader.crc32 = fileInfo.crc32;
        localHeader.compressedSize = compressedData.size();
        localHeader.uncompressedSize = fileInfo.data.size();

        QByteArray fileNameBytes = fileInfo.relativePath.toUtf8();
        localHeader.fileNameLength = fileNameBytes.size();
        localHeader.extraFieldLength = 0;

        // 记录本地头位置
        quint32 localHeaderOffset = zipFile.pos();

        // 写入本地头
        out.writeRawData(reinterpret_cast<const char*>(&localHeader), sizeof(localHeader));
        out.writeRawData(fileNameBytes.constData(), fileNameBytes.size());

        // 写入文件数据
        out.writeRawData(compressedData.constData(), compressedData.size());

        // 创建中央目录记录
        CentralDirRecord cdRecord;
        CentralDirHeader cdHeader;
        cdHeader.signature = 0x02014b50;
        cdHeader.versionMadeBy = 20;
        cdHeader.versionNeeded = 20;
        cdHeader.flags = 0;
        cdHeader.compressionMethod = localHeader.compressionMethod;
        cdHeader.lastModTime = localHeader.lastModTime;
        cdHeader.lastModDate = localHeader.lastModDate;
        cdHeader.crc32 = localHeader.crc32;
        cdHeader.compressedSize = localHeader.compressedSize;
        cdHeader.uncompressedSize = localHeader.uncompressedSize;
        cdHeader.fileNameLength = localHeader.fileNameLength;
        cdHeader.extraFieldLength = 0;
        cdHeader.fileCommentLength = 0;
        cdHeader.diskNumberStart = 0;
        cdHeader.internalFileAttr = 0;
        cdHeader.externalFileAttr = 0;
        cdHeader.localHeaderOffset = localHeaderOffset;

        // 将中央目录头和数据序列化
        QByteArray cdData;
        QDataStream cdStream(&cdData, QIODevice::WriteOnly);
        cdStream.setByteOrder(QDataStream::LittleEndian);
        cdStream.writeRawData(reinterpret_cast<const char*>(&cdHeader), sizeof(cdHeader));
        cdStream.writeRawData(fileNameBytes.constData(), fileNameBytes.size());

        cdRecord.data = cdData;
        cdRecord.localHeaderOffset = localHeaderOffset;
        m_centralRecords.append(cdRecord);

        emit progressChanged(i + 1, m_files.size(), fileInfo.relativePath);
    }

    // 写入中央目录
    quint32 centralDirOffset = zipFile.pos();
    for (const CentralDirRecord& record : TAS_CONST(m_centralRecords)) {
        out.writeRawData(record.data.constData(), record.data.size());
    }
    quint32 centralDirSize = zipFile.pos() - centralDirOffset;

    // 写入目录结束标记
    EndOfCentralDir eocd;
    eocd.signature = 0x06054b50;
    eocd.diskNumber = 0;
    eocd.diskWithCentralDir = 0;
    eocd.centralDirEntriesOnDisk = m_centralRecords.size();
    eocd.totalCentralDirEntries = m_centralRecords.size();
    eocd.centralDirSize = centralDirSize;
    eocd.centralDirOffset = centralDirOffset;
    eocd.commentLength = 0;

    out.writeRawData(reinterpret_cast<const char*>(&eocd), sizeof(eocd));

    zipFile.close();

    m_files.clear();
    m_centralRecords.clear();

    emit compressionFinished(true);
    return true;
}

bool QZipStream::unCompress(const QString& zipFile, const QString& outDir)
{
    m_lastError.clear();

    QFile file(zipFile);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QString("无法打开zip文件: %1").arg(zipFile);
        emit errorOccurred(m_lastError);
        return false;
    }

    // 创建输出目录
    QDir outputDir(outDir);
    if (!outputDir.exists()) {
        outputDir.mkpath(".");
    }

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);

    // 查找中央目录结束标记
    file.seek(file.size() - sizeof(EndOfCentralDir));

    EndOfCentralDir eocd;
    in.readRawData(reinterpret_cast<char*>(&eocd), sizeof(eocd));

    if (eocd.signature != 0x06054b50) {
        // 没有找到目录结束标记，尝试从文件末尾扫描
        file.seek(qMax((qint64)0, file.size() - 65536));   // 扫描最后64KB
        QByteArray tail = file.readAll();
        int pos = tail.lastIndexOf("PK\x05\x06");   // 目录结束标记

        if (pos == -1) {
            m_lastError = "无效的zip文件格式";
            emit errorOccurred(m_lastError);
            file.close();
            return false;
        }

        file.seek(file.size() - tail.size() + pos);
        in.readRawData(reinterpret_cast<char*>(&eocd), sizeof(eocd));
    }

    // 定位到中央目录开始
    file.seek(eocd.centralDirOffset);

    // 先收集所有文件信息
    QList<CentralDirHeader> centralHeaders;
    QList<QString> fileNames;

    for (quint16 i = 0; i < eocd.totalCentralDirEntries; i++) {
        CentralDirHeader cdHeader;
        in.readRawData(reinterpret_cast<char*>(&cdHeader), sizeof(cdHeader));

        if (cdHeader.signature != 0x02014b50) {
            m_lastError = "中央目录记录损坏";
            emit errorOccurred(m_lastError);
            break;
        }

        // 读取文件名
        QByteArray fileNameBytes(cdHeader.fileNameLength, 0);
        in.readRawData(fileNameBytes.data(), cdHeader.fileNameLength);
        QString fileName = QString::fromUtf8(fileNameBytes);

        // 跳过额外字段和注释
        file.seek(file.pos() + cdHeader.extraFieldLength + cdHeader.fileCommentLength);

        // 存储文件信息
        centralHeaders.append(cdHeader);
        fileNames.append(fileName);
    }

    // 解压所有文件
    for (int i = 0; i < centralHeaders.size(); i++) {
        const CentralDirHeader& cdHeader = centralHeaders[i];
        const QString& fileName = fileNames[i];

        // 定位到本地文件头
        if (!file.seek(cdHeader.localHeaderOffset)) {
            m_lastError = QString("无法定位到文件头: %1").arg(fileName);
            emit errorOccurred(m_lastError);
            continue;
        }

        LocalFileHeader localHeader;
        in.readRawData(reinterpret_cast<char*>(&localHeader), sizeof(localHeader));

        if (localHeader.signature != 0x04034b50) {
            m_lastError = QString("本地文件头损坏: %1").arg(fileName);
            emit errorOccurred(m_lastError);
            continue;
        }

        // 读取文件名
        QByteArray localFileNameBytes(localHeader.fileNameLength, 0);
        in.readRawData(localFileNameBytes.data(), localHeader.fileNameLength);
        QString localFileName = QString::fromUtf8(localFileNameBytes);

        // 检查文件名是否匹配
        if (localFileName != fileName) {
            m_lastError = QString("文件名不匹配: 中央目录[%1] vs 本地头[%2]").arg(fileName).arg(localFileName);
            emit errorOccurred(m_lastError);
            continue;
        }

        // 跳过额外字段
        file.seek(file.pos() + localHeader.extraFieldLength);

        // 读取压缩数据
        QByteArray compressedData(localHeader.compressedSize, 0);
        qint64 bytesRead = in.readRawData(compressedData.data(), localHeader.compressedSize);

        if (bytesRead != localHeader.compressedSize) {
            m_lastError = QString("读取压缩数据失败: %1 (期望%2字节, 实际%3字节)")
                              .arg(fileName)
                              .arg(localHeader.compressedSize)
                              .arg(bytesRead);
            emit errorOccurred(m_lastError);
            continue;
        }

        // 验证CRC（可选）
        if (localHeader.crc32 != cdHeader.crc32) {
            m_lastError = QString("CRC校验失败: %1 (本地头:%2, 中央目录:%3)")
                              .arg(fileName)
                              .arg(localHeader.crc32, 8, 16, QChar('0'))
                              .arg(cdHeader.crc32, 8, 16, QChar('0'));
            qDebug() << m_lastError;
            // 继续处理，不一定立即失败
        }

        // 检查大小是否一致
        if (localHeader.compressedSize != cdHeader.compressedSize || localHeader.uncompressedSize != cdHeader.uncompressedSize) {
            m_lastError = QString("文件大小不一致: %1").arg(fileName);
            qDebug() << m_lastError;
            // 使用中央目录的大小信息
        }

        // 解压数据
        QByteArray uncompressedData;
        bool decompressSuccess = true;

        if (localHeader.compressionMethod == 0) {
            // 不压缩
            uncompressedData = compressedData;
        } else if (localHeader.compressionMethod == 8) {
            // DEFLATE压缩
            quint32 uncompressedSize = cdHeader.uncompressedSize;
            uncompressedData.resize(uncompressedSize);

            z_stream stream;
            stream.zalloc = Z_NULL;
            stream.zfree = Z_NULL;
            stream.opaque = Z_NULL;
            stream.avail_in = compressedData.size();
            stream.next_in = reinterpret_cast<Bytef*>(compressedData.data());
            stream.avail_out = uncompressedData.size();
            stream.next_out = reinterpret_cast<Bytef*>(uncompressedData.data());

            if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
                m_lastError = QString("解压初始化失败: %1").arg(fileName);
                emit errorOccurred(m_lastError);
                decompressSuccess = false;
            } else {
                int ret = inflate(&stream, Z_FINISH);
                inflateEnd(&stream);

                if (ret != Z_STREAM_END) {
                    m_lastError = QString("解压失败: %1 (错误码:%2)").arg(fileName).arg(ret);
                    emit errorOccurred(m_lastError);
                    decompressSuccess = false;
                } else if (stream.total_out != uncompressedSize) {
                    m_lastError =
                        QString("解压大小不匹配: %1 (期望%2, 实际%3)").arg(fileName).arg(uncompressedSize).arg(stream.total_out);
                    qDebug() << m_lastError;
                    uncompressedData.resize(stream.total_out);
                }
            }
        } else {
            m_lastError = QString("不支持的压缩方法(%1): %2").arg(localHeader.compressionMethod).arg(fileName);
            emit errorOccurred(m_lastError);
            decompressSuccess = false;
        }

        if (!decompressSuccess) {
            continue;   // 跳过这个文件，继续处理下一个
        }

        // 创建输出文件路径
        QString outputPath = outputDir.filePath(localFileName);
        QFileInfo fileInfo(outputPath);

        // 创建目录
        if (!QDir().mkpath(fileInfo.absolutePath())) {
            m_lastError = QString("无法创建目录: %1").arg(fileInfo.absolutePath());
            emit errorOccurred(m_lastError);
            continue;
        }

        // 写入文件
        QFile outFile(outputPath);
        if (outFile.open(QIODevice::WriteOnly)) {
            qint64 written = outFile.write(uncompressedData);
            outFile.close();

            if (written != uncompressedData.size()) {
                m_lastError =
                    QString("写入文件不完整: %1 (期望%2, 实际%3)").arg(outputPath).arg(uncompressedData.size()).arg(written);
                emit errorOccurred(m_lastError);
            } else {
                qDebug() << "成功解压文件:" << outputPath;

                // 设置文件时间（从DOS时间恢复）
                QDateTime fileTime = QDateTime::currentDateTime();
                // 这里可以添加从DOS时间恢复文件时间的代码

                emit progressChanged(i + 1, centralHeaders.size(), fileName);
            }
        } else {
            m_lastError = QString("无法创建文件: %1").arg(outputPath);
            emit errorOccurred(m_lastError);
        }
    }

    file.close();

    if (centralHeaders.isEmpty()) {
        m_lastError = "没有找到可解压的文件";
        emit errorOccurred(m_lastError);
        emit compressionFinished(false);
        return false;
    }

    emit compressionFinished(true);
    return true;
}

bool QZipStream::addFileInternal(const QString& filePath, const QString& relativePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        m_lastError = QString("无法打开文件: %1").arg(filePath);
        emit errorOccurred(m_lastError);
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    if (data.isEmpty()) {
        qDebug() << "文件为空:" << filePath;
        return true;   // 空文件也添加
    }

    FileInfo fileInfo;
    fileInfo.relativePath = relativePath;
    fileInfo.data = data;
    fileInfo.crc32 = crc32(0, reinterpret_cast<const Bytef*>(data.constData()), data.size());
    fileInfo.lastModified = QFileInfo(filePath).lastModified();
    fileInfo.dosTime = toDosTime(fileInfo.lastModified);
    fileInfo.dosDate = toDosDate(fileInfo.lastModified);

    m_files.append(fileInfo);
    return true;
}

// 修复的压缩函数 - 使用deflate生成raw DEFLATE数据
bool QZipStream::compressDataDeflate(const QByteArray& input, QByteArray& output, CompressionLevel level)
{
    if (input.isEmpty()) {
        output.clear();
        return true;
    }

    z_stream stream;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;

    // 使用deflateInit2指定生成raw DEFLATE数据（不带zlib头）
    // -MAX_WBITS表示使用原始deflate格式
    if (deflateInit2(&stream, level, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        return false;
    }

    // 计算最大压缩大小
    uLongf maxSize = deflateBound(&stream, input.size());
    output.resize(maxSize);

    stream.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(input.constData()));
    stream.avail_in = input.size();
    stream.next_out = reinterpret_cast<Bytef*>(output.data());
    stream.avail_out = maxSize;

    int ret = deflate(&stream, Z_FINISH);
    if (ret != Z_STREAM_END) {
        deflateEnd(&stream);
        return false;
    }

    output.resize(stream.total_out);
    deflateEnd(&stream);

    return true;
}

quint16 QZipStream::toDosTime(const QDateTime& dt)
{
    return ((dt.time().hour() & 0x1F) << 11) | ((dt.time().minute() & 0x3F) << 5) | ((dt.time().second() / 2) & 0x1F);
}

quint16 QZipStream::toDosDate(const QDateTime& dt)
{
    int year = dt.date().year() - 1980;
    if (year < 0)
        year = 0;
    if (year > 127)
        year = 127;

    return ((year & 0x7F) << 9) | ((dt.date().month() & 0x0F) << 5) | (dt.date().day() & 0x1F);
}

void QZipStream::setCompressionLevel(CompressionLevel level)
{
    m_compressionLevel = level;
}

QZipStream::CompressionLevel QZipStream::compressionLevel() const
{
    return m_compressionLevel;
}

QString QZipStream::lastError() const
{
    return m_lastError;
}

int QZipStream::currentProgress() const
{
    return m_currentProgress;
}

int QZipStream::totalProgress() const
{
    return m_totalProgress;
}

QString QZipStream::currentFileName() const
{
    return m_currentFile;
}