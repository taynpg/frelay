#ifndef CLIENTCORE_H
#define CLIENTCORE_H

#include <InfoClient.h>
#include <InfoDirFile.h>
#include <Protocol.h>
#include <QDataStream>
#include <QHostAddress>
#include <QMutex>
#include <QMutexLocker>
#include <QTcpSocket>
#include <QThread>

class ClientCore : public QObject
{
    Q_OBJECT

public:
    ClientCore(QObject* parent = nullptr);
    ~ClientCore();

public:
    bool Connect(const QString& ip, quint16 port);
    void Disconnect();
    bool Send(QSharedPointer<FrameBuffer> frame);
    bool Send(const char* data, qint64 len);

private:
    void onReadyRead();
    void onDisconnected();

private:
    void UseFrame(QSharedPointer<FrameBuffer> frame);

public:
    void SetClientsCall(const std::function<void(const InfoClientVec& clients)>& call);
    void SetPathCall(const std::function<void(const QString& path)>& call);
    void SetFileCall(const std::function<void(const DirFileInfoVec& files)>& call);
    void SetRemoteID(const QString& id);
    QString GetRemoteID();

public:
    QMutex conMutex_;
    QString remoteID_;
    QTcpSocket* socket_;
    QByteArray recvBuffer_;

    std::function<void(const QString& path)> pathCall_;
};

#endif   // CLIENTCORE_H