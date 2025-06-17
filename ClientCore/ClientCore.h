#ifndef CLIENTCORE_H
#define CLIENTCORE_H

#include <InfoClient.h>
#include <InfoDirFile.h>
#include <InfoMsg.h>
#include <InfoPack.hpp>
#include <LocalFile.h>
#include <Protocol.h>
#include <QDataStream>
#include <QHostAddress>
#include <QMutex>
#include <QMutexLocker>
#include <QTcpSocket>
#include <QThread>
#include <array>

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
    template <typename T> bool Send(const T& info, FrameBufferType type, const QString& tid)
    {
        auto f = QSharedPointer<FrameBuffer>::create();
        f->tid = tid;
        f->data = infoPack<T>(info);
        f->type = type;
        return Send(f);
    }

private:
    void onReadyRead();
    void onDisconnected();

private:
    void UseFrame(QSharedPointer<FrameBuffer> frame);

public:
    void SetClientsCall(const std::function<void(const InfoClientVec& clients)>& call);
    void SetPathCall(const std::function<void(const QString& path)>& call);
    void SetFileCall(const std::function<void(const DirFileInfoVec& files)>& call);
    void SetFrameCall(FrameBufferType type, const std::function<void(QSharedPointer<FrameBuffer>)>& call);
    void SetRemoteID(const QString& id);
    QString GetRemoteID();

public:
    QMutex conMutex_;
    QString ownID_;
    QString remoteID_;

    QMutex sockMut_;
    QTcpSocket* socket_;
    QByteArray recvBuffer_;

    LocalFile localFile_;

    std::function<void(const QString& path)> pathCall_;
    std::function<void(const InfoClientVec& clients)> clientsCall_;
    std::function<void(const DirFileInfoVec& files)> fileCall_;

    std::array<std::function<void(QSharedPointer<FrameBuffer>)>, 256> frameCall_;
};

#endif   // CLIENTCORE_H