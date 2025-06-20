﻿#include "Util.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QStandardPaths>
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

#include <QDir>
#include <QFileInfo>

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

void GlobalData::SetRemoteRoot(const QString& root)
{
    QMutexLocker locker(&mutex_);
	RemoteRoot_ = root;
}

QString GlobalData::GetLocalRoot() const
{
    return LocalRoot_;
}

QString GlobalData::GetRemoteRoot() const
{
    return RemoteRoot_;
}