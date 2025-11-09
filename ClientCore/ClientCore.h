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
#include <QQueue>
#include <QTcpSocket>
#include <QThread>
#include <array>
#include <QReadWriteLock>

class ClientCore : public QObject
{
    Q_OBJECT

public:
    ClientCore(QObject* parent = nullptr);
    ~ClientCore();

public slots:
    bool SendFrame(QSharedPointer<FrameBuffer> frame);
    void Disconnect();

public slots:
    void DoConnect(const QString& ip, quint16 port);

public:
    void Instance();
    bool Connect(const QString& ip, quint16 port);
    bool Send(QSharedPointer<FrameBuffer> frame);
    bool Send(const char* data, qint64 len);
    bool IsConnect();
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
    static bool syncInvoke(ClientCore* context, QSharedPointer<FrameBuffer> frame)
    {
        bool result = false;
        bool success = QMetaObject::invokeMethod(context, "SendFrame", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result),
                                                 Q_ARG(QSharedPointer<FrameBuffer>, frame));
        if (!success) {
            return false;
        }
        return result;
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
    void sigFileInfo(QSharedPointer<FrameBuffer> frame);
    void sigOffline(QSharedPointer<FrameBuffer> frame);
    void sigYourId(QSharedPointer<FrameBuffer> frame);
    void sigMsgAsk(QSharedPointer<FrameBuffer> frame);
    void sigMsgAnswer(QSharedPointer<FrameBuffer> frame);
    void sigFlowBack(QSharedPointer<FrameBuffer> frame);
    void sigTransInterrupt(QSharedPointer<FrameBuffer> frame);

signals:
    void conSuccess();
    void connecting();
    void conFailed();

private:
    void onReadyRead();
    void onDisconnected();
    void handleAsk(QSharedPointer<FrameBuffer> frame);

private:
    void UseFrame(QSharedPointer<FrameBuffer> frame);

public:
    void SetRemoteID(const QString& id);
    QString GetRemoteID();
    QString GetOwnID();
    void pushID(const QString& id);
    void popID(const QString& id);

public:
    QMutex conMutex_;
    QString ownID_;

    // 这是被动发送时，对方ID。
    QReadWriteLock rwIDLock_;
    QVector<QString> remoteIDs_;

    // 这是主动通信时的对方ID。
    QString remoteID_;

    QMutex sockMut_;
    QTcpSocket* socket_{};
    QByteArray recvBuffer_;

    bool connected_{false};
    LocalFile localFile_;
};

// Socket Worker Thread
class SocketWorker : public QThread
{
    Q_OBJECT

public:
    SocketWorker(ClientCore* core, QObject* parent = nullptr);
    ~SocketWorker();

protected:
    void run() override;

private:
    ClientCore* core_{};
};

// HeatBeat to Server
class HeatBeat : public QThread
{
    Q_OBJECT

public:
    HeatBeat(ClientCore* core, QObject* parent = nullptr);
    ~HeatBeat() override;

public:
    void Stop();

protected:
    void run() override;

private:
    bool isRun_{false};
    ClientCore* core_{};
};

#endif   // CLIENTCORE_H