// qzip.cpp
#include "QZip.h"

#include <JlCompress.h>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <quazip.h>
#include <quazipfile.h>

QZip::QZip(QObject* parent) : QObject(parent)
{
    clearError();
}

QZip::~QZip()
{
}

void QZip::clearError()
{
    m_lastError.clear();
    m_lastErrorCode = 0;
}

void QZip::setError(int code, const QString& message)
{
    m_lastErrorCode = code;
    m_lastError = message;
    qWarning() << "QZip Error [" << code << "]: " << message;
}

QString QZip::lastError() const
{
    return m_lastError;
}

int QZip::lastErrorCode() const
{
    return m_lastErrorCode;
}

bool QZip::compressFiles(const QString& zipPath, const QStringList& filePaths)
{
    clearError();

    if (zipPath.isEmpty() || filePaths.isEmpty()) {
        setError(-1, "Invalid parameters: zip path or file list is empty");
        return false;
    }

    QFileInfo zipInfo(zipPath);
    if (zipInfo.exists()) {
        if (!QFile::remove(zipPath)) {
            setError(-2, QString("Cannot remove existing file: %1").arg(zipPath));
            return false;
        }
    }

    // 准备文件路径列表
    QStringList realFilePaths;
    foreach (const QString& filePath, filePaths) {
        QFileInfo info(filePath);
        if (!info.exists()) {
            setError(-3, QString("File does not exist: %1").arg(filePath));
            return false;
        }
        realFilePaths.append(info.absoluteFilePath());
    }

    // 使用JlCompress进行压缩
    bool result = JlCompress::compressFiles(zipPath, realFilePaths);

    if (!result) {
        setError(-4, QString("Failed to compress files to: %1").arg(zipPath));
    } else {
        emit operationCompleted(true, QString("Successfully compressed %1 files").arg(filePaths.size()));
    }

    return result;
}

bool QZip::compressDirectory(const QString& zipPath, const QString& dirPath, bool recursive)
{
    clearError();

    if (zipPath.isEmpty() || dirPath.isEmpty()) {
        setError(-1, "Invalid parameters: zip path or directory path is empty");
        return false;
    }

    QFileInfo zipInfo(zipPath);
    if (zipInfo.exists()) {
        if (!QFile::remove(zipPath)) {
            setError(-2, QString("Cannot remove existing file: %1").arg(zipPath));
            return false;
        }
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        setError(-3, QString("Directory does not exist: %1").arg(dirPath));
        return false;
    }

    // 使用JlCompress进行目录压缩
    bool result = JlCompress::compressDir(zipPath, dirPath, recursive);

    if (!result) {
        setError(-4, QString("Failed to compress directory: %1").arg(dirPath));
    } else {
        emit operationCompleted(true, QString("Successfully compressed directory: %1").arg(dirPath));
    }

    return result;
}

bool QZip::extractFile(const QString& zipPath, const QString& targetFileName, const QString& destPath)
{
    clearError();

    if (zipPath.isEmpty() || targetFileName.isEmpty()) {
        setError(-1, "Invalid parameters: zip path or target file name is empty");
        return false;
    }

    QFileInfo zipInfo(zipPath);
    if (!zipInfo.exists()) {
        setError(-2, QString("Zip file does not exist: %1").arg(zipPath));
        return false;
    }

    QString destination = destPath.isEmpty() ? QDir::currentPath() : destPath;
    QDir destDir(destination);
    if (!destDir.exists()) {
        if (!destDir.mkpath(destination)) {
            setError(-3, QString("Cannot create destination directory: %1").arg(destination));
            return false;
        }
    }

    // 使用JlCompress解压单个文件
    QString extractedFile = JlCompress::extractFile(zipPath, targetFileName, destination);

    if (extractedFile.isEmpty()) {
        setError(-4, QString("Failed to extract file: %1 from %2").arg(targetFileName).arg(zipPath));
        return false;
    }

    emit operationCompleted(true, QString("Successfully extracted: %1").arg(targetFileName));
    return true;
}

bool QZip::extractFiles(const QString& zipPath, const QStringList& fileNames, const QString& destDir)
{
    clearError();

    if (zipPath.isEmpty() || fileNames.isEmpty()) {
        setError(-1, "Invalid parameters: zip path or file list is empty");
        return false;
    }

    QFileInfo zipInfo(zipPath);
    if (!zipInfo.exists()) {
        setError(-2, QString("Zip file does not exist: %1").arg(zipPath));
        return false;
    }

    QString destination = destDir.isEmpty() ? QDir::currentPath() : destDir;
    QDir destDirObj(destination);
    if (!destDirObj.exists()) {
        if (!destDirObj.mkpath(destination)) {
            setError(-3, QString("Cannot create destination directory: %1").arg(destination));
            return false;
        }
    }

    // 使用JlCompress解压多个文件
    QStringList extractedFiles = JlCompress::extractFiles(zipPath, fileNames, destination);

    if (extractedFiles.isEmpty()) {
        setError(-4, QString("Failed to extract files from: %1").arg(zipPath));
        return false;
    }

    emit operationCompleted(true, QString("Successfully extracted %1 files").arg(extractedFiles.size()));
    return true;
}

bool QZip::extractAll(const QString& zipPath, const QString& destDir)
{
    clearError();

    if (zipPath.isEmpty()) {
        setError(-1, "Invalid parameters: zip path is empty");
        return false;
    }

    QFileInfo zipInfo(zipPath);
    if (!zipInfo.exists()) {
        setError(-2, QString("Zip file does not exist: %1").arg(zipPath));
        return false;
    }

    QString destination = destDir.isEmpty() ? QDir::currentPath() : destDir;
    QDir destDirObj(destination);
    if (!destDirObj.exists()) {
        if (!destDirObj.mkpath(destination)) {
            setError(-3, QString("Cannot create destination directory: %1").arg(destination));
            return false;
        }
    }

    // 使用JlCompress解压整个压缩包
    QStringList extractedFiles = JlCompress::extractDir(zipPath, destination);

    if (extractedFiles.isEmpty()) {
        setError(-4, QString("Failed to extract files from: %1").arg(zipPath));
        return false;
    }

    emit operationCompleted(true, QString("Successfully extracted %1 files/directories").arg(extractedFiles.size()));
    return true;
}

QStringList QZip::getFileList(const QString& zipPath)
{
    clearError();

    if (zipPath.isEmpty()) {
        setError(-1, "Invalid parameters: zip path is empty");
        return QStringList();
    }

    QFileInfo zipInfo(zipPath);
    if (!zipInfo.exists()) {
        setError(-2, QString("Zip file does not exist: %1").arg(zipPath));
        return QStringList();
    }

    // 使用JlCompress获取文件列表
    QStringList fileList = JlCompress::getFileList(zipPath);

    if (fileList.isEmpty()) {
        setError(-3, QString("Cannot get file list from: %1").arg(zipPath));
    }

    return fileList;
}