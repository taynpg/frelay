// qzip.cpp
#include "QZip.h"

#include <JlCompress.h>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <quazip.h>
#include <quazipfile.h>

// 辅助函数：递归收集文件夹中的所有文件
void collectAllFiles(const QString& dirPath, QList<QPair<QString, QString>>& fileList, const QString& basePath = QString())
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return;
    }

    // 遍历当前目录下的所有文件
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    for (const QFileInfo& file : files) {
        QString relativePath = basePath.isEmpty() ? file.fileName() : basePath + "/" + file.fileName();
        fileList.append(qMakePair(file.absoluteFilePath(), relativePath));
    }

    // 递归遍历子目录
    QFileInfoList dirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& subDir : dirs) {
        QString newBasePath = basePath.isEmpty() ? subDir.fileName() : basePath + "/" + subDir.fileName();

        // 先添加目录本身（如果需要的话）
        // fileList.append(qMakePair(subDir.absoluteFilePath() + "/", newBasePath + "/"));

        // 递归收集子目录中的文件
        collectAllFiles(subDir.absoluteFilePath(), fileList, newBasePath);
    }
}

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

void QZip::collectFilesFromDir(const QString& dirPath, QStringList& fileList, const QString& relativePath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return;
    }

    // 遍历所有文件
    QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::Hidden | QDir::System);
    for (const QFileInfo& file : files) {
        QString newRelativePath = relativePath.isEmpty() ? file.fileName() : relativePath + "/" + file.fileName();
        fileList.append(file.absoluteFilePath());
    }

    // 递归遍历子目录
    QFileInfoList dirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QFileInfo& subDir : dirs) {
        QString newRelativePath = relativePath.isEmpty() ? subDir.fileName() : relativePath + "/" + subDir.fileName();

        // 先添加目录本身（如果需要的话）
        // fileList.append(subDir.absoluteFilePath() + "/");

        // 递归收集子目录中的文件
        collectFilesFromDir(subDir.absoluteFilePath(), fileList, newRelativePath);
    }
}

bool QZip::compress(const QString& zipPath, const QString& dirRoot, const QStringList& subDirs, const QStringList& subFiles)
{
    clearError();

    // 参数验证
    if (zipPath.isEmpty() || dirRoot.isEmpty()) {
        setError(-1, "Invalid parameters: zip path or root directory is empty");
        return false;
    }

    if (subDirs.isEmpty() && subFiles.isEmpty()) {
        setError(-2, "No subdirectories or files specified for compression");
        return false;
    }

    QDir rootDir(dirRoot);
    if (!rootDir.exists()) {
        setError(-3, QString("Root directory does not exist: %1").arg(dirRoot));
        return false;
    }

    // 检查目标压缩包是否存在
    QFileInfo zipInfo(zipPath);
    if (zipInfo.exists()) {
        if (!QFile::remove(zipPath)) {
            setError(-4, QString("Cannot remove existing zip file: %1").arg(zipPath));
            return false;
        }
    }

    // 创建QuaZip实例
    QuaZip zip(zipPath);
    if (!zip.open(QuaZip::mdCreate)) {
        setError(-5, QString("Cannot create zip file: %1").arg(zipPath));
        return false;
    }

    // 收集所有要压缩的文件
    QList<QPair<QString, QString>> allFiles;   // <绝对路径, 压缩包内相对路径>

    // 1. 处理指定的子文件夹（递归收集所有文件）
    for (const QString& subDir : subDirs) {
        QString fullSubDirPath = rootDir.absoluteFilePath(subDir);
        QFileInfo subDirInfo(fullSubDirPath);

        if (!subDirInfo.exists()) {
            setError(-5, QString("Subdirectory does not exist: %1").arg(fullSubDirPath));
            zip.close();
            return false;
        }

        if (!subDirInfo.isDir()) {
            setError(-6, QString("Path is not a directory: %1").arg(fullSubDirPath));
            zip.close();
            return false;
        }

        // 递归收集子文件夹中的所有文件
        collectAllFiles(fullSubDirPath, allFiles, subDir);
    }

    // 2. 处理指定的文件
    for (const QString& subFile : subFiles) {
        QString fullFilePath = rootDir.absoluteFilePath(subFile);
        QFileInfo fileInfo(fullFilePath);

        if (!fileInfo.exists()) {
            setError(-7, QString("File does not exist: %1").arg(fullFilePath));
            zip.close();
            return false;
        }

        if (!fileInfo.isFile()) {
            setError(-8, QString("Path is not a file: %1").arg(fullFilePath));
            zip.close();
            return false;
        }

        allFiles.append(qMakePair(fullFilePath, subFile));
    }

    if (allFiles.isEmpty()) {
        setError(-9, "No valid files found to compress");
        zip.close();
        return false;
    }

    qDebug() << "Found" << allFiles.size() << "files to compress";

    // 3. 开始压缩所有文件
    int processedFiles = 0;
    int totalFiles = allFiles.size();

    for (const auto& filePair : allFiles) {
        QString fileFullPath = filePair.first;
        QString zipFilePath = filePair.second;

        QFile file(fileFullPath);
        if (!file.open(QIODevice::ReadOnly)) {
            setError(-10, QString("Cannot open file: %1").arg(fileFullPath));
            zip.close();
            return false;
        }

        // 确保父目录在压缩包中存在
        QString parentDir = zipFilePath.section('/', 0, -2);
        if (!parentDir.isEmpty()) {
            // 在压缩包中创建目录（如果需要的话）
            // 对于QuaZip，添加空目录需要在添加文件时自动处理
        }

        QuaZipFile zipFile(&zip);
        QuaZipNewInfo newInfo(zipFilePath, fileFullPath);

        if (!zipFile.open(QIODevice::WriteOnly, newInfo)) {
            setError(-11, QString("Cannot add file to zip: %1").arg(zipFilePath));
            file.close();
            zip.close();
            return false;
        }

        // 复制文件内容
        QByteArray buffer = file.readAll();
        if (zipFile.write(buffer) != buffer.size()) {
            setError(-12, QString("Failed to write file to zip: %1").arg(zipFilePath));
            zipFile.close();
            file.close();
            zip.close();
            return false;
        }

        zipFile.close();
        file.close();

        processedFiles++;
        emit progressChanged(processedFiles, totalFiles);

        qDebug() << "Compressed: " << zipFilePath;
    }

    zip.close();

    if (zip.getZipError() != ZIP_OK) {
        setError(-13, QString("Zip creation failed with error: %1").arg(zip.getZipError()));
        return false;
    }

    emit operationCompleted(true, QString("Successfully compressed %1 items").arg(processedFiles));
    return true;
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