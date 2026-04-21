#include "QZip.h"

#include <QDateTime>
#include <quazip/JlCompress.h>
#include <quazip/quazipfile.h>

QZip::QZip(QObject* parent)
    : QObject(parent), m_zipFile(nullptr), m_compressionLevel(5)   // 默认压缩级别
      ,
      m_isCompressing(false)
{
}

QZip::~QZip()
{
    if (m_zipFile) {
        endCompress();
    }
}

bool QZip::startCompress(const QString& outFile)
{
    m_lastError.clear();   // 清空之前的错误信息

    if (m_isCompressing) {
        setError("Compression already in progress!");
        return false;
    }

    // 检查输出文件路径是否有效
    if (outFile.isEmpty()) {
        setError("Output file path is empty!");
        return false;
    }

    // 确保输出目录存在
    QFileInfo fileInfo(outFile);
    QDir outDir = fileInfo.absoluteDir();
    if (!outDir.exists()) {
        if (!outDir.mkpath(".")) {
            setError(QString("Failed to create output directory: %1").arg(outDir.path()));
            return false;
        }
    }

    // 创建QuaZip对象
    m_zipFile = new QuaZip(outFile);
    if (!m_zipFile) {
        setError("Failed to create QuaZip object: Memory allocation failed");
        return false;
    }

    // 打开压缩文件
    if (!m_zipFile->open(QuaZip::mdCreate)) {
        QString errorMsg = QString("Failed to open zip file for writing: %1").arg(outFile);
        if (m_zipFile->getZipError() != 0) {
            errorMsg += QString(" (Error code: %1)").arg(m_zipFile->getZipError());
        }
        setError(errorMsg);

        delete m_zipFile;
        m_zipFile = nullptr;
        return false;
    }

    m_outFilePath = outFile;
    m_isCompressing = true;

    qDebug() << "Compression started, output file:" << outFile;
    return true;
}

bool QZip::addFolder(const QString& dir)
{
    m_lastError.clear();

    if (!m_isCompressing || !m_zipFile) {
        setError("Compression not started or invalid state!");
        return false;
    }

    QDir sourceDir(dir);
    if (!sourceDir.exists()) {
        setError(QString("Source directory does not exist: %1").arg(dir));
        return false;
    }

    return addFolderRecursive(dir);
}

bool QZip::addFile(const QString& file)
{
    m_lastError.clear();

    if (!m_isCompressing || !m_zipFile) {
        setError("Compression not started or invalid state!");
        return false;
    }

    QFileInfo fileInfo(file);
    if (!fileInfo.exists()) {
        setError(QString("File does not exist: %1").arg(file));
        return false;
    }

    if (fileInfo.isDir()) {
        return addFolder(file);
    }

    QFile inFile(file);
    if (!inFile.open(QIODevice::ReadOnly)) {
        setError(QString("Failed to open file for reading: %1").arg(file));
        return false;
    }

    // 设置压缩文件中的相对路径
    QString relativePath = fileInfo.fileName();

    QuaZipFile outFile(m_zipFile);
    QuaZipNewInfo newInfo(relativePath, file);

    // 设置文件修改时间
    newInfo.setFileDateTime(file);

    if (!outFile.open(QIODevice::WriteOnly, newInfo, nullptr, 0, m_compressionLevel)) {
        setError(QString("Failed to add file to zip: %1").arg(file));
        inFile.close();
        return false;
    }

    // 写入文件内容
    QByteArray data = inFile.readAll();
    if (outFile.write(data) != data.size()) {
        setError(QString("Failed to write file data to zip: %1").arg(file));
        outFile.close();
        inFile.close();
        return false;
    }

    outFile.close();
    inFile.close();

    qDebug() << "Added file to zip:" << file;
    return true;
}

bool QZip::addFolderRecursive(const QString& dir, const QString& baseDir)
{
    QDir sourceDir(dir);
    if (!sourceDir.exists()) {
        setError(QString("Directory does not exist: %1").arg(dir));
        return false;
    }

    // 遍历目录中的所有条目
    QFileInfoList entries = sourceDir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

    foreach (const QFileInfo& entry, entries) {
        QString relativePath = baseDir.isEmpty() ? entry.fileName() : baseDir + "/" + entry.fileName();

        if (entry.isDir()) {
            // 对于目录，在zip中创建一个目录条目
            QuaZipFile outFile(m_zipFile);
            QuaZipNewInfo newInfo(relativePath + "/", entry.absoluteFilePath());
            newInfo.setFileDateTime(entry.absolutePath());

            if (outFile.open(QIODevice::WriteOnly, newInfo, nullptr, 0, m_compressionLevel)) {
                outFile.close();
            } else {
                setError(QString("Failed to add directory entry to zip: %1").arg(relativePath));
                return false;
            }

            // 递归处理子目录
            if (!addFolderRecursive(entry.absoluteFilePath(), relativePath)) {
                return false;
            }
        } else {
            // 处理文件
            QFile inFile(entry.absoluteFilePath());
            if (!inFile.open(QIODevice::ReadOnly)) {
                setError(QString("Failed to open file for reading: %1").arg(entry.absoluteFilePath()));
                continue;
            }

            QuaZipFile outFile(m_zipFile);
            QuaZipNewInfo newInfo(relativePath, entry.absoluteFilePath());
            newInfo.setFileDateTime(entry.absolutePath());

            if (!outFile.open(QIODevice::WriteOnly, newInfo, nullptr, 0, m_compressionLevel)) {
                setError(QString("Failed to add file to zip: %1").arg(entry.absoluteFilePath()));
                inFile.close();
                continue;
            }

            // 写入文件内容
            QByteArray data = inFile.readAll();
            if (outFile.write(data) != data.size()) {
                setError(QString("Failed to write file data to zip: %1").arg(entry.absoluteFilePath()));
            } else {
                qDebug() << "Added file to zip:" << relativePath;
            }

            outFile.close();
            inFile.close();
        }
    }

    return true;
}

bool QZip::endCompress()
{
    m_lastError.clear();

    if (!m_isCompressing || !m_zipFile) {
        setError("Compression not started or invalid state!");
        return false;
    }

    bool result = true;

    // 关闭压缩文件
    m_zipFile->close();
    int zipError = m_zipFile->getZipError();
    if (zipError != UNZ_OK) {
        setError(QString("Error closing zip file. Error code: %1").arg(zipError));
        result = false;
    }

    delete m_zipFile;
    m_zipFile = nullptr;
    m_isCompressing = false;

    qDebug() << "Compression completed:" << (result ? "Success" : "Failed");
    return result;
}

bool QZip::unCompress(const QString& zipFile, const QString& outDir)
{
    m_lastError.clear();

    if (zipFile.isEmpty() || outDir.isEmpty()) {
        setError("Invalid parameters: zip file or output directory is empty!");
        return false;
    }

    QFileInfo zipInfo(zipFile);
    if (!zipInfo.exists()) {
        setError(QString("Zip file does not exist: %1").arg(zipFile));
        return false;
    }

    // 确保输出目录存在
    QDir outputDir(outDir);
    if (!outputDir.exists()) {
        if (!outputDir.mkpath(".")) {
            setError(QString("Failed to create output directory: %1").arg(outDir));
            return false;
        }
    }

    // 记录开始时间
    QDateTime startTime = QDateTime::currentDateTime();

    // 使用JlCompress进行解压
    QStringList extractedFiles = JlCompress::extractDir(zipFile, outDir);

    if (extractedFiles.isEmpty()) {
        setError(QString("Failed to extract zip file or zip file is empty: %1").arg(zipFile));

        // 检查是否是文件权限问题
        if (!QFileInfo(zipFile).isReadable()) {
            m_lastError += " (File is not readable)";
        }

        return false;
    }

    // 计算解压时间
    qint64 elapsed = startTime.msecsTo(QDateTime::currentDateTime());

    qDebug() << "Extracted" << extractedFiles.size() << "files to:" << outDir << "in" << elapsed << "ms";

    return true;
}

void QZip::setCompressionLevel(int level)
{
    m_compressionLevel = qBound(0, level, 9);
    qDebug() << "Compression level set to:" << m_compressionLevel;
}

QString QZip::lastError() const
{
    return m_lastError;
}

void QZip::setError(const QString& error)
{
    m_lastError = error;
    qWarning() << "QZip Error:" << error;
}