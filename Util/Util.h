#ifndef UTIL_H
#define UTIL_H

#include <InfoDirFile.h>
#include <QObject>

class Util : public QObject
{
    Q_OBJECT
public:
    Util();

public:
    static QString GetUserHome();
    static void InitLogger(const QString& logPath, const QString& mark);
    static QString Get2FilePath(const QString& file, const QString& directory);
    static QString Join(const QString& path, const QString& name);
    static void ConsoleMsgHander(QtMsgType type, const QMessageLogContext& context, const QString& msg);
};

class DirFileHelper : public QObject
{
    Q_OBJECT
public:
    DirFileHelper(QObject* parent = nullptr);
    virtual ~DirFileHelper() = default;

public:
    QString GetErr() const;

signals:
    void sigHome(const QString& path);
    void sigDirFile(const DirFileInfoVec& dirFile);

protected:
    QString err_;

public:
    virtual bool GetHome() = 0;
    virtual bool GetDirFile(const QString& dir) = 0;
};

#endif   // UTIL_H