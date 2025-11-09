#ifndef UTIL_H
#define UTIL_H

#include <InfoDirFile.h>
#include <QMutex>
#include <QObject>

class GlobalData
{
public:
    static GlobalData* Ins();
    ~GlobalData() = default;

public:
    void SetLocalRoot(const QString& root);
    void SetLocalID(const QString& id);
    void SetRemoteRoot(const QString& root);
    void SetRemoteID(const QString& id);
    QString GetLocalRoot() const;
    QString GetLocalID() const;
    QString GetRemoteRoot() const;
    QString GetRemoteID() const;
    std::string GetConfigPath() const;
    void SetConfigPath(const std::string& path);

private:
    GlobalData() = default;

public:
    QMutex mutex_;
    QString LocalRoot_;
    QString RemoteRoot_;
    QString LocalID_;
    QString RemoteID_;
    std::string ConfigPath_;
};

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
    static QString GetCurConfigPath(const QString& sub);
    static void ConsoleMsgHander(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static QString GetVersion();
    static bool FileExist(const QString& path);
    static bool DirExist(const QString& path, bool isFilePath);
    static QString UUID();
    static QVector<QString> GetLocalDrivers();
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
    void sigHome(const QString& path, const QStringList& drivers);
    void sigDirFile(const DirFileInfoVec& dirFile);

protected:
    QString err_;

public:
    virtual bool GetHome() = 0;
    virtual bool GetDirFile(const QString& dir) = 0;
};

#endif   // UTIL_H