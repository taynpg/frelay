#ifndef CLIENTCORE_H
#define CLIENTCORE_H

#include <InfoClient.h>
#include <InfoDirFile.h>
#include <InfoMsg.h>
#include <InfoPack.hpp>
#include <LocalFile.h>
#include <Protocol.h>
#include <QDataStream>
#include <QFuture>
#include <QHostAddress>
#include <QMutex>
#include <QMutexLocker>
#include <QPromise>
#include <QQueue>
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
        auto f = GetBuffer<T>(info, type, tid);
        return Send(f);
    }
    template <typename T> QSharedPointer<FrameBuffer> GetBuffer(const T& info, FrameBufferType type, const QString& tid)
    {
        auto f = QSharedPointer<FrameBuffer>::create();
        f->tid = tid;
        f->data = infoPack<T>(info);
        f->type = type;
        return f;
    }
    template <typename Callable> static bool asyncInvoke(QObject* context, Callable&& func)
    {
        auto promise = QSharedPointer<QPromise<bool>>::create();
        QFuture<bool> future = promise->future();

        QMetaObject::invokeMethod(
            context,
            [func = std::forward<Callable>(func), promise]() mutable {
                try {
                    promise->addResult(func());
                } catch (...) {
                    promise->addResult(false);
                }
                promise->finish();
            },
            Qt::QueuedConnection);

        future.waitForFinished();
        return future.result();
    }

signals:
    void sigDisconnect();
    void sigPath(const QString& path);
    void sigClients(const InfoClientVec& clients);
    void sigFiles(const DirFileInfoVec& files);
    void sigReqSend(QSharedPointer<FrameBuffer> frame);
    void sigReqDown(QSharedPointer<FrameBuffer> frame);
    void sigTransDone(QSharedPointer<FrameBuffer> frame);
    void sigCanSend(QSharedPointer<FrameBuffer> frame);
    void sigCanotSend(QSharedPointer<FrameBuffer> frame);
    void sigCanotDown(QSharedPointer<FrameBuffer> frame);
    void sigCanDown(QSharedPointer<FrameBuffer> frame);
    void sigFileBuffer(QSharedPointer<FrameBuffer> frame);
    void sigTransFailed(QSharedPointer<FrameBuffer> frame);

private:
    void onReadyRead();
    void onDisconnected();

private:
    void UseFrame(QSharedPointer<FrameBuffer> frame);

public:
    void SetRemoteID(const QString& id);
    QString GetRemoteID();
    QString GetOwnID();

public:
    QMutex conMutex_;
    QString ownID_;
    QString remoteID_;

    QMutex sockMut_;
    QTcpSocket* socket_;
    QByteArray recvBuffer_;

    LocalFile localFile_;
};

class SocketWorker : public QThread
{
    Q_OBJECT

public:
    SocketWorker(ClientCore* core, QObject* parent = nullptr);
    ~SocketWorker();

public:
    void SetConnectInfo(const QString& ip, qint16 port);

protected:
    void run() override;

signals:
    void conSuccess();
    void connecting();
    void conFailed();
    void disconnected();

private:
    QString ip_;
    qint16 port_{};
    ClientCore* core_{};
};

#endif   // CLIENTCORE_H