#ifndef UTIL_H
#define UTIL_H

#include <QObject>
#include <InfoDirFile.h>

class Util : public QObject
{
    Q_OBJECT
public:
    Util();

public:
    static QString GetUserHome();
    static void InitLogger(const QString& logPath, const QString& mark);
    static QString Get2FilePath(const QString& file, const QString& directory);
    static void ConsoleMsgHander(QtMsgType type, const QMessageLogContext& context, const QString& msg);
};

class DirFileHelper : public QObject {
    Q_OBJECT
public:
    DirFileHelper() = default;
    virtual ~DirFileHelper() = default;

public:
    QString GetErr() const;
    void registerPathCall(const std::function<void(const QString& path)>& call);
    void registerFileCall(const std::function<void(const DirFileInfoVec& vec)>& call);

protected:
    QString err_;
    std::function<void(const QString& path)> pathCall_;
    std::function<void(const DirFileInfoVec& info)> fileCall_;

public:
    virtual bool GetHome() = 0;
    virtual bool GetDirFile(const QString& dir) = 0;
};

#endif   // UTIL_H