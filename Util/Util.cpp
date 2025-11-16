#include "Util.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileDevice>
#include <QFileInfo>
#include <QMutex>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QUuid>
#include <fversion.h>
#include <iostream>
#include <spdlog/fmt/bundled/color.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static QMutex msgMutex;
static std::shared_ptr<spdlog::logger> logger;

Util::Util()
{
}

QString Util::GetUserHome()
{
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (homePath.isEmpty()) {
        qWarning() << "Failed to get user home directory";
        homePath = QDir::homePath();
    }
    return homePath;
}

QString Util::GetCurConfigPath(const QString& sub)
{
    // Get user's home directory
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    if (homePath.isEmpty()) {
        qWarning() << "Failed to get user home directory";
        return QString();
    }

    auto configPath = QDir(homePath).absoluteFilePath(".config");
    // Append subdirectory if provided
    if (!sub.isEmpty()) {
        configPath = QDir(configPath).absoluteFilePath(sub);
    }

    // Create the directory if it doesn't exist
    QDir dir(configPath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qWarning() << "Failed to create config directory:" << configPath;
            return QString();
        }
    }

    return QDir::cleanPath(configPath);
}

QString Util::Join(const QString& path, const QString& name)
{
    return QDir::cleanPath(path + QDir::separator() + name);
}

void Util::InitLogger(const QString& logPath, const QString& mark)
{
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logPath.toStdString(), 1024 * 1024 * 50, 3);
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%l]: %v");
    console_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e][%l]: %v%$");
    std::vector<spdlog::sink_ptr> sinks{file_sink, console_sink};
    logger = std::make_shared<spdlog::logger>(mark.toStdString(), sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::debug);
    spdlog::register_logger(logger);
}

// do not check exit
QString Util::Get2FilePath(const QString& file, const QString& directory)
{
    if (file.isEmpty() || directory.isEmpty()) {
        return QString();
    }

    QString fileName = QFileInfo(file).fileName();
    QString cleanDir = QDir::cleanPath(directory);
    QString fullPath = QDir(cleanDir).filePath(fileName);

    return QDir::cleanPath(fullPath);
}

void Util::ConsoleMsgHander(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);

    switch (type) {
    case QtDebugMsg:
        logger->debug(msg.toStdString());
        break;
    case QtInfoMsg:
        logger->info(msg.toStdString());
        break;
    case QtWarningMsg:
        logger->warn(msg.toStdString());
        break;
    case QtCriticalMsg:
        logger->error(msg.toStdString());
        break;
    case QtFatalMsg:
        logger->critical(msg.toStdString());
        break;
    default:
        logger->warn("Unknown QtMsgType type.");
        break;
    }
}

QString Util::GetVersion()
{
    auto ver = QString("frelay %1 %2 %3").arg(VERSION_NUM, VERSION_DEV, VERSION_GIT_COMMIT);
    return ver;
}

bool Util::FileExist(const QString& path)
{
    return QFile::exists(path);
}

bool Util::DirExist(const QString& path, bool isFilePath)
{
    if (path.isEmpty()) {
        return false;
    }

    QString dirPath = path;
    if (isFilePath) {
        QFileInfo fileInfo(path);
        dirPath = fileInfo.absolutePath();
    } else {
        QFileInfo dirInfo(path);
        dirPath = dirInfo.absoluteFilePath();
    }
    QDir dir(dirPath);
    return dir.exists();
}

QString Util::Rename(const QString& from, const QString& to, bool isDir)
{
    if (isDir) {
        QDir dir;
        if (dir.rename(from, to)) {
            return "";
        }
        return tr("请确认是否无权限操作或者被占用。");
    } else {
        QFile f(from);
        if (f.rename(to)) {
            return "";
        }
        return f.errorString();
    }
}

QString Util::UUID()
{
    return QUuid::createUuid().toString().remove("{").remove("}");
}

QVector<QString> Util::GetLocalDrivers()
{
    QVector<QString> result;
    auto drivers = QStorageInfo::mountedVolumes();
    for (const auto& driver : drivers) {
        if (driver.isValid() && driver.isReady()) {
            result.push_back(driver.rootPath());
        }
    }
    return result;
}

QString DirFileHelper::GetErr() const
{
    return QString();
}

DirFileHelper::DirFileHelper(QObject* parent) : QObject(parent)
{
}

GlobalData* GlobalData::Ins()
{
    static GlobalData instance;
    return &instance;
}

void GlobalData::SetLocalRoot(const QString& root)
{
    QMutexLocker locker(&mutex_);
    LocalRoot_ = root;
}

void GlobalData::SetLocalID(const QString& id)
{
    QMutexLocker locker(&mutex_);
    LocalID_ = id;
}

void GlobalData::SetRemoteID(const QString& id)
{
    QMutexLocker locker(&mutex_);
    RemoteID_ = id;
}

void GlobalData::SetRemoteRoot(const QString& root)
{
    QMutexLocker locker(&mutex_);
    RemoteRoot_ = root;
}

QString GlobalData::GetLocalRoot() const
{
    return LocalRoot_;
}

QString GlobalData::GetLocalID() const
{
    return LocalID_;
}

QString GlobalData::GetRemoteRoot() const
{
    return RemoteRoot_;
}

QString GlobalData::GetRemoteID() const
{
    return RemoteID_;
}

std::string GlobalData::GetConfigPath() const
{
    return ConfigPath_;
}

void GlobalData::SetConfigPath(const std::string& path)
{
    ConfigPath_ = path;
}

QString Util::Delete(const QString& path)
{
    QFileInfo fileInfo(path);

    if (!fileInfo.exists()) {
        return tr("文件或目录不存在: %1").arg(path);
    }

    if (fileInfo.isFile()) {
        QFile file(path);
        if (file.remove()) {
            return "";
        } else {
            return tr("删除文件失败: %1").arg(file.errorString());
        }
    } else if (fileInfo.isDir()) {
        QDir dir(path);
        if (dir.removeRecursively()) {
            return "";
        } else {
            return tr("删除目录失败，确认是否占用或者无权限操作。");
        }
    } else {
        return tr("不支持的文件类型: %1").arg(path);
    }
}

QString Util::NewDir(const QString& path)
{
    if (path.isEmpty()) {
        return tr("路径不能为空");
    }

    if (QDir(path).exists()) {
        return tr("目录已存在: %1").arg(path);
    }

    QFileInfo pathInfo(path);
    QDir parentDir = pathInfo.absoluteDir();

    if (!parentDir.exists()) {
        return tr("父目录不存在: %1").arg(parentDir.absolutePath());
    }

    QFileInfo parentInfo(parentDir.absolutePath());
    if (!parentInfo.isWritable()) {
        return tr("父目录无写入权限: %1").arg(parentDir.absolutePath());
    }

    QDir dir;
    if (dir.mkpath(path)) {
        return "";
    } else {
        return tr("创建目录失败: %1").arg(path);
    }
}