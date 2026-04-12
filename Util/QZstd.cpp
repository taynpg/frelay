// QZstdStream.cpp
#include <QDebug>
#include <QFile>
#include <zstd.h>

#include "QZstd.h"

class QZstdStream::Private
{
public:
    ZSTD_CCtx* compressCtx = nullptr;
    ZSTD_DCtx* decompressCtx = nullptr;
    QByteArray outputBuffer;
    QScopedPointer<QFile> outputFile;
    QString lastError;

    bool isCompressing = false;
    bool isDecompressing = false;

    ~Private()
    {
        cleanup();
    }

    void cleanup()
    {
        if (compressCtx) {
            ZSTD_freeCCtx(compressCtx);
            compressCtx = nullptr;
        }
        if (decompressCtx) {
            ZSTD_freeDCtx(decompressCtx);
            decompressCtx = nullptr;
        }
        outputFile.reset();
        isCompressing = false;
        isDecompressing = false;
    }

    bool writeCompressedData(const void* data, size_t size)
    {
        if (outputFile) {
            return outputFile->write(static_cast<const char*>(data), size) == static_cast<qint64>(size);
        } else {
            outputBuffer.append(static_cast<const char*>(data), size);
            return true;
        }
    }
};

QZstdStream::QZstdStream(QObject* parent) : QObject(parent), d(new Private)
{
}

QZstdStream::~QZstdStream()
{
    delete d;
}

bool QZstdStream::startCompress(CompressionLevel level)
{
    d->cleanup();

    d->compressCtx = ZSTD_createCCtx();
    if (!d->compressCtx) {
        d->lastError = "Failed to create compression context";
        return false;
    }

    ZSTD_CCtx_setParameter(d->compressCtx, ZSTD_c_compressionLevel, level);
    d->outputBuffer.clear();
    d->isCompressing = true;
    d->lastError.clear();

    return true;
}

bool QZstdStream::startCompressToFile(const QString& filePath, CompressionLevel level)
{
    if (!startCompress(level)) {
        return false;
    }

    d->outputFile.reset(new QFile(filePath));
    if (!d->outputFile->open(QIODevice::WriteOnly)) {
        d->lastError = QString("Cannot open file: %1").arg(filePath);
        d->cleanup();
        return false;
    }

    return true;
}

bool QZstdStream::compress(const QByteArray& data)
{
    if (!d->isCompressing || !d->compressCtx) {
        d->lastError = "Compression not started";
        return false;
    }

    if (data.isEmpty()) {
        return true;
    }

    // 压缩数据
    size_t bound = ZSTD_compressBound(data.size());
    QByteArray compressed(bound, 0);

    size_t compressedSize =
        ZSTD_compressCCtx(d->compressCtx, compressed.data(), bound, data.constData(), data.size(), ZSTD_e_continue);

    if (ZSTD_isError(compressedSize)) {
        d->lastError = QString("Compression failed: %1").arg(ZSTD_getErrorName(compressedSize));
        return false;
    }

    // 写入压缩数据大小和数据
    compressed.resize(compressedSize);

    quint32 size = static_cast<quint32>(compressedSize);
    if (!d->writeCompressedData(&size, sizeof(size))) {
        d->lastError = "Failed to write compressed data size";
        return false;
    }

    if (!d->writeCompressedData(compressed.constData(), compressedSize)) {
        d->lastError = "Failed to write compressed data";
        return false;
    }

    return true;
}

QByteArray QZstdStream::endCompress()
{
    if (!d->isCompressing || !d->compressCtx) {
        d->lastError = "Compression not started";
        return QByteArray();
    }

    // 刷新压缩器
    size_t bound = 1024;   // 小缓冲区用于刷新
    QByteArray compressed(bound, 0);

    size_t compressedSize = ZSTD_compressCCtx(d->compressCtx, compressed.data(), bound, nullptr, 0, ZSTD_e_end);

    if (!ZSTD_isError(compressedSize) && compressedSize > 0) {
        compressed.resize(compressedSize);
        quint32 size = static_cast<quint32>(compressedSize);
        d->writeCompressedData(&size, sizeof(size));
        d->writeCompressedData(compressed.constData(), compressedSize);
    }

    d->cleanup();

    if (d->outputFile) {
        d->outputFile->close();
        return QByteArray();   // 文件模式返回空
    }

    return d->outputBuffer;
}

bool QZstdStream::startDecompress()
{
    d->cleanup();

    d->decompressCtx = ZSTD_createDCtx();
    if (!d->decompressCtx) {
        d->lastError = "Failed to create decompression context";
        return false;
    }

    d->isDecompressing = true;
    d->lastError.clear();

    return true;
}

bool QZstdStream::startDecompressFromFile(const QString& filePath)
{
    d->cleanup();

    d->outputFile.reset(new QFile(filePath));
    if (!d->outputFile->open(QIODevice::ReadOnly)) {
        d->lastError = QString("Cannot open file: %1").arg(filePath);
        return false;
    }

    d->isDecompressing = true;
    d->lastError.clear();

    return true;
}

QByteArray QZstdStream::decompress(const QByteArray& data)
{
    if (!d->isDecompressing || !d->decompressCtx) {
        d->lastError = "Decompression not started";
        return QByteArray();
    }

    if (data.isEmpty()) {
        return QByteArray();
    }

    const char* ptr = data.constData();
    size_t remaining = data.size();

    if (remaining < sizeof(quint32)) {
        d->lastError = "Insufficient data for decompression";
        return QByteArray();
    }

    QByteArray result;

    while (remaining >= sizeof(quint32)) {
        // 读取压缩数据大小
        quint32 compressedSize = *reinterpret_cast<const quint32*>(ptr);
        ptr += sizeof(quint32);
        remaining -= sizeof(quint32);

        if (remaining < compressedSize) {
            d->lastError = "Incomplete compressed data";
            return QByteArray();
        }

        // 解压数据
        const size_t maxDecompressedSize = 1024 * 1024;   // 1MB限制
        QByteArray decompressed(maxDecompressedSize, 0);

        size_t decompressedSize =
            ZSTD_decompressDCtx(d->decompressCtx, decompressed.data(), maxDecompressedSize, ptr, compressedSize);

        if (ZSTD_isError(decompressedSize)) {
            d->lastError = QString("Decompression failed: %1").arg(ZSTD_getErrorName(decompressedSize));
            return QByteArray();
        }

        decompressed.resize(decompressedSize);
        result.append(decompressed);

        ptr += compressedSize;
        remaining -= compressedSize;
    }

    return result;
}

bool QZstdStream::endDecompress()
{
    if (!d->isDecompressing) {
        d->lastError = "Decompression not started";
        return false;
    }

    d->cleanup();
    return true;
}

QString QZstdStream::errorString() const
{
    return d->lastError;
}