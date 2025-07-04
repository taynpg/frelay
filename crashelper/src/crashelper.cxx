#include "crashelper.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>

#include "backward.hpp"

#ifdef _WIN32
std::function<std::string()> backward::SignalHandling::crash_path_getter_ = nullptr;
std::function<void(EXCEPTION_POINTERS* info)> backward::SignalHandling::crash_use_handler_ = nullptr;
std::function<void(int sig)> backward::SignalHandling::user_sig_handler_ = nullptr;
#else
std::function<std::string()> backward::SignalHandling::crash_path_getter_ = nullptr;
std::function<void(int sig)> backward::SignalHandling::user_sig_handler_ = nullptr;
#endif

namespace backward {

class crashHelper
{
public:
    static crashHelper& getInstance()
    {
        static crashHelper instance;
        return instance;
    }

public:
    static bool createDir(const QString& path);
    static QString getCurrentTime();
    static QString getBinName();
    static QString toFull(const QString& path);

public:
    QString dumpSavePath_;
    QString dumpLogSavePath_;

private:
    crashHelper() = default;
    ~crashHelper() = default;
};

bool crashHelper::createDir(const QString& path)
{
    QDir dir(path);
    if (dir.exists()) {
        if (!dir.isReadable()) {
            return false;
        }
        return true;
    }

    if (!dir.mkpath(".")) {
        return false;
    }

    return true;
}

QString crashHelper::getCurrentTime()
{
    QDateTime now = QDateTime::currentDateTime();
    return now.toString("yyyyMMdd-HHmmss-zzz");
}

QString crashHelper::getBinName()
{
    QString exePath = QCoreApplication::applicationFilePath();
    if (!exePath.isEmpty()) {
        QFileInfo fi(exePath);
        return fi.completeBaseName();   // Returns the name without extension
    }
    return "";
}

QString crashHelper::toFull(const QString& path)
{
    QFileInfo fi(path);
    if (fi.isRelative()) {
        return QDir::current().absoluteFilePath(path);
    }
    return fi.absoluteFilePath();
}

bool SetDumpFileSavePath(const QString& path)
{
    auto& h = crashHelper::getInstance();
    if (!h.createDir(path)) {
        return false;
    }
    h.dumpSavePath_ = path;
    return true;
}

bool SetDumpLogSavePath(const QString& path)
{
    auto& h = crashHelper::getInstance();
    if (!h.createDir(path)) {
        return false;
    }
    h.dumpLogSavePath_ = path;
    return true;
}

std::string GetCurFullLogPath()
{
    auto gf = []() {
        auto& h = crashHelper::getInstance();
        QString dumpName = h.getCurrentTime() + "-" + h.getBinName() + ".log";
        QString fullPath = QDir(h.dumpLogSavePath_).absoluteFilePath(dumpName);
        return fullPath.toStdString();
    };
#if defined(WIN_OS)
#if !defined(NDEBUG) || defined(_DEBUG) || defined(DEBUG)
    return gf();
#else
    return "";
#endif
#else
    return gf();
#endif
}

#if defined(WIN_OS)
void UseExceptionHandler(EXCEPTION_POINTERS* exception)
{
    auto& h = crashHelper::getInstance();
    // Release mode, output dump result to file
    QString dumpBase = h.getCurrentTime() + "-" + h.getBinName();
    QString dumpName = h.getCurrentTime() + "-" + h.getBinName() + ".windump";
    QString dumpFailedLog = h.getCurrentTime() + "-" + h.getBinName() + ".dumpfailed.log";
    QString fullPath = QDir(h.dumpSavePath_).absoluteFilePath(dumpName);
    QString fullFailedPath = QDir(h.dumpSavePath_).absoluteFilePath(dumpFailedLog);

    HANDLE hFile =
        CreateFileA(fullPath.toStdString().c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        QFile file(fullFailedPath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "Create dump file failed.\n";
            out << "File: " << fullPath << "\n";
            out << "Error code: " << GetLastError() << "\n";
        }
        return;
    };

    MINIDUMP_EXCEPTION_INFORMATION info;
    info.ThreadId = GetCurrentThreadId();
    info.ExceptionPointers = exception;
    info.ClientPointers = TRUE;
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &info, NULL, NULL);
    CloseHandle(hFile);
}
#endif
}   // namespace backward
