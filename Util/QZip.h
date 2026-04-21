#ifndef QZIP_H
#define QZIP_H

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QObject>
#include <QString>
#include <quazip/quazip.h>

class QZip : public QObject
{
    Q_OBJECT

public:
    explicit QZip(QObject* parent = nullptr);
    ~QZip();

    // 压缩相关函数
    bool startCompress(const QString& outFile);
    bool addFolder(const QString& dir);
    bool addFile(const QString& file);
    bool endCompress();

    // 解压相关函数
    bool unCompress(const QString& zipFile, const QString& outDir);

    // 设置压缩级别 (0-9, 0=不压缩, 9=最大压缩)
    void setCompressionLevel(int level);

    // 获取最后一次错误信息
    QString lastError() const;

private:
    // 递归添加文件夹
    bool addFolderRecursive(const QString& dir, const QString& baseDir = "");
    
    // 设置错误信息
    void setError(const QString& error);

private:
    QuaZip* m_zipFile;        // 压缩文件对象
    QString m_outFilePath;    // 输出文件路径
    int m_compressionLevel;   // 压缩级别
    bool m_isCompressing;     // 是否正在压缩
    QString m_lastError;      // 最后一次错误信息
};

#endif   // QZIP_H