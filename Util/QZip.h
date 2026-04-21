// qzip.h
#ifndef QZIP_H
#define QZIP_H

#include <QObject>
#include <QString>
#include <QStringList>

class QZip : public QObject
{
    Q_OBJECT

public:
    explicit QZip(QObject* parent = nullptr);
    virtual ~QZip();

    // 压缩相关方法
    bool compressFiles(const QString& zipPath, const QStringList& filePaths);
    bool compressDirectory(const QString& zipPath, const QString& dirPath, bool recursive = true);

    // 新增函数：压缩指定目录下的特定子文件夹和文件
    bool compress(const QString& zipPath, const QString& dirRoot, const QStringList& subDirs, const QStringList& subFiles);

    // 解压相关方法
    bool extractFile(const QString& zipPath, const QString& targetFileName, const QString& destPath = QString());
    bool extractFiles(const QString& zipPath, const QStringList& fileNames, const QString& destDir = QString());
    bool extractAll(const QString& zipPath, const QString& destDir = QString());

    // 工具方法
    QStringList getFileList(const QString& zipPath);

    // 错误处理
    QString lastError() const;
    int lastErrorCode() const;

signals:
    void progressChanged(int current, int total);
    void operationCompleted(bool success, const QString& message);

private:
    QString m_lastError;
    int m_lastErrorCode = 0;

    // 辅助函数：收集指定目录下的所有文件
    void collectFilesFromDir(const QString& dirPath, QStringList& fileList, const QString& relativePath = QString());

    // 清除错误状态
    void clearError();

    // 设置错误
    void setError(int code, const QString& message);
};

#endif   // QZIP_H