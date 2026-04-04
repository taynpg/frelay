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
#include <QReadWriteLock>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <array>

class WaitOperOwn;
struct WaitTask {
    QString id;
    WaitOperOwn* wo;
};
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
    static void asyncInvoke(ClientCore* context, QSharedPointer<FrameBuffer> frame)
    {
        bool success = QMetaObject::invokeMethod(context, "SendFrame",
                                                 Qt::QueuedConnection,
                                                 Q_ARG(QSharedPointer<FrameBuffer>, frame));

        if (!success) {
            qWarning() << "Failed to invoke SendFrame asynchronously";
        }
    }

signals:
    void sigDisconnect();
    void sigPath(const QString& path, const QVector<QString>& drivers);
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
    void clearWaitTask();

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

    QTimer* clearWaitTimer_{};
    QMutex waitTaskMut_;
    QMap<QString, WaitTask> waitTask_;
};

// 工作线程。
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

// 心跳包线程。
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

// 耗时操作线程基本框架。
class WaitThread : public QThread
{
    Q_OBJECT

public:
    WaitThread(QObject* parent = nullptr);

public:
    void SetClient(ClientCore* cli);
    bool IsQuit() const;

Q_SIGNALS:
    void sigCheckOver();

public Q_SLOTS:
    virtual void interrupCheck();
    virtual void recvFrame(QSharedPointer<FrameBuffer> frame) = 0;

protected:
    bool isRun_;
    bool isAlreadyInter_;
    ClientCore* cli_{};
};

// 等待对方应答的等待线程。
class WaitOper : public WaitThread
{
public:
    WaitOper(QObject* parent = nullptr);

public:
    void run() override;
    void SetType(const QString& sendType, const QString& ansType);
    void SetPath(const QString& stra, const QString& strb, const QString& type);
    InfoMsg GetMsgConst() const;
    InfoMsg& GetMsgRef();
    void interrupCheck() override;
    void recvFrame(QSharedPointer<FrameBuffer> frame) override;

private:
    bool recvMsg_{};
    InfoMsg infoMsg_{};
    QString sendStrType_{};
    QString ansStrType_{};
    QString stra_;
    QString strb_;
    QString type_;
};

// 等待自己耗时操作的线程。
class WaitOperOwn : public WaitThread
{
    Q_OBJECT

public:
    WaitOperOwn(QObject* parent = nullptr);

signals:
    void sigOver();

public:
    void run() override;
    void recvFrame(QSharedPointer<FrameBuffer> frame) override;

public:
    QString fid;
    InfoMsg infoMsg_{};
    std::function<bool()> func_;
};

#endif   // CLIENTCORE_H
