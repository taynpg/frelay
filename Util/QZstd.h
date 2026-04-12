// QZstdStream.h
#pragma once
#include <QByteArray>
#include <QIODevice>
#include <QObject>

class QZstdStream : public QObject
{
    Q_OBJECT

public:
    explicit QZstdStream(QObject* parent = nullptr);
    ~QZstdStream();

    // 压缩级别
    enum CompressionLevel {
        Fastest = 1,
        Fast = 3,
        Default = 5,
        GoodCompression = 9,
        MaxCompression = 19
    };

    // 开始压缩（输出到内存）
    bool startCompress(CompressionLevel level = Default);

    // 开始压缩到文件
    bool startCompressToFile(const QString& filePath, CompressionLevel level = Default);

    // 压缩数据块
    bool compress(const QByteArray& data);

    // 结束压缩
    QByteArray endCompress();

    // 开始解压
    bool startDecompress();

    // 开始从文件解压
    bool startDecompressFromFile(const QString& filePath);

    // 解压数据块
    QByteArray decompress(const QByteArray& data);

    // 结束解压
    bool endDecompress();

    // 获取错误信息
    QString errorString() const;

private:
    class Private;
    Private* d;
};