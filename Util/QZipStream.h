#pragma once
#include <QByteArray>
#include <QDateTime>
#include <QObject>
#include <QString>

class QZipStream : public QObject
{
    Q_OBJECT

public:
    explicit QZipStream(QObject* parent = nullptr);
    ~QZipStream();

    // 压缩级别 (0=不压缩, 1=最快, 9=最高压缩)
    enum CompressionLevel {
        NoCompression = 0,
        Fastest = 1,
        Fast = 3,
        Default = 6,
        Maximum = 9
    };

public:
    // 压缩
    bool startCompress(const QString& outFile);
    bool addFolder(const QString& dir);
    bool addFile(const QString& file);
    bool endCompress();

    // 解压
    bool unCompress(const QString& zipFile, const QString& outDir);

    // 设置压缩级别
    void setCompressionLevel(CompressionLevel level);
    CompressionLevel compressionLevel() const;

    // 错误信息
    QString lastError() const;

    // 进度信息
    int currentProgress() const;
    int totalProgress() const;
    QString currentFileName() const;

signals:
    void progressChanged(int current, int total, const QString& currentFile);
    void errorOccurred(const QString& error);
    void compressionStarted();
    void compressionFinished(bool success);
    void fileAdded(const QString& filePath, qint64 size);

private:
    // 内部数据结构
    struct FileInfo {
        QString relativePath;     // 相对路径
        QByteArray data;          // 原始数据
        quint32 crc32;            // CRC32校验
        QDateTime lastModified;   // 最后修改时间
        quint16 dosTime;          // DOS格式时间
        quint16 dosDate;          // DOS格式日期
    };

    struct CentralDirRecord {
        QByteArray data;             // 中央目录记录数据
        quint32 localHeaderOffset;   // 本地头偏移
    };

private:
    // 私有方法
    bool addFileInternal(const QString& filePath, const QString& relativePath = QString());
    bool compressDataDeflate(const QByteArray& input, QByteArray& output, CompressionLevel level);
    quint16 toDosTime(const QDateTime& dt);
    quint16 toDosDate(const QDateTime& dt);

private:
    QString m_outputFileName;                        // 输出文件名
    QString m_lastError;                             // 最后错误信息
    QString m_currentFile;                           // 当前处理文件
    CompressionLevel m_compressionLevel = Default;   // 压缩级别
    QList<FileInfo> m_files;                         // 文件列表
    QList<CentralDirRecord> m_centralRecords;        // 中央目录记录
    int m_currentProgress = 0;                       // 当前进度
    int m_totalProgress = 0;                         // 总进度
};