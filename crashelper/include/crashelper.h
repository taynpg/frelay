#ifndef CRASHELPER_H
#define CRASHELPER_H

#if defined(_WIN32) || defined(_WIN64)
#define WIN_OS
#include <windows.h>
#if defined(min)
#undef min
#endif
#elif defined(__APPLE__) && defined(__MACH__)
#define MAC_OS
#elif defined(__linux__) || defined(__TERMUX__) || defined(TERMUX) || defined(__ANDROID__)
#define LINUX_OS
#define BACKWARD_HAS_BFD 1
#else
#error "Unsupported OS"
#endif

#include "backward.hpp"
#include <QString>

namespace backward {
///
/// @brief 设置dump文件的保存目录，不设置默认当前目录。
/// @param path 若不存在则创建，创建失败返回false
///
bool SetDumpFileSavePath(const QString& path);

///
/// @brief 设置dump日志文件的保存目录，不设置默认当前目录。
/// @param path 若不存在则创建，创建失败返回false
///
bool SetDumpLogSavePath(const QString& path);

std::string GetCurFullLogPath();
#if defined(WIN_OS)
void UseExceptionHandler(EXCEPTION_POINTERS* exception);
#endif

}   // namespace backward

#ifdef WIN_OS
#define CRASHELPER_MARK_ENTRY()                                                                                        \
    backward::SignalHandling sh;                                                                                       \
    sh.register_crash_use_handler([](EXCEPTION_POINTERS* exception) { backward::UseExceptionHandler(exception); });    \
    sh.register_crash_path([]() -> std::string { return backward::GetCurFullLogPath(); })
#elif defined(MAC_OS)
#elif defined(LINUX_OS)
#define CRASHELPER_MARK_ENTRY()                                                                                        \
    backward::SignalHandling sh;                                                                                       \
    sh.register_crash_path([]() -> std::string { return backward::GetCurFullLogPath(); })
#endif

#endif   // CRASHELPER_H