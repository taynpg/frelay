#ifndef CLIENTTHREAD_H
#define CLIENTTHREAD_H

#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QTcpSocket>
#include <QThread>
#include <QWaitCondition>
#include <memory>

#include "Protocol.h"

class Server;
class ClientThread : public QObject
{
    Q_OBJECT

public:
    explicit ClientThread(const QString& id, Server* server, QObject* parent = nullptr);
    ~ClientThread();

    bool start(QTcpSocket* socket);

             // 线程安全的队列操作
    bool queueDataForSending(const QSharedPointer<FrameBuffer>& frame);

    QString getId() const;
    qint64 getLastHeartbeatTime() const;

    bool isActive() const;
    bool isConnected() const;

signals:
    void disconnected(const QString& clientId);
    void dataReceived(const QString& clientId, const QSharedPointer<FrameBuffer>& frame);
    void heartbeatReceived(const QString& clientId);
    void heartbeatTimeout(const QString& clientId, qint64 lastTime);

public slots:
    void onReadyRead();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    bool syncSendFrame(const QSharedPointer<FrameBuffer>& frame);
    bool directSend(QSharedPointer<FrameBuffer> frame);
    void stop();
    void updateHeartbeatTime();

private slots:
    void processReceiveData();
    void processSendData();

private:
    void initSocketConnections();
    void cleanupThreads();

    QTcpSocket* socket_;
    QString id_;
    Server* server_;

             // 状态变量，需要互斥保护
    bool active_;
    qint64 lastHeartbeatTime_;
    mutable QMutex socketMutex_;

             // 接收缓冲区
    QByteArray receiveByteBuffer_;
    QMutex bufferMutex_;

             // 接收帧队列
    QQueue<QSharedPointer<FrameBuffer>> receiveFrameQueue_;
    QMutex frameQueueMutex_;
    QWaitCondition frameQueueNotEmpty_;
    QWaitCondition frameQueueNotFull_;
    bool stopReceiveThread_;
    QThread* receiveThread_;

             // 发送帧队列
    QQueue<QSharedPointer<FrameBuffer>> sendFrameQueue_;
    QMutex sendQueueMutex_;
    QWaitCondition sendQueueNotEmpty_;
    QWaitCondition sendQueueNotFull_;
    bool stopSendThread_;
    QThread* sendThread_;

    QObject* recvWorker_;
    QObject* sendWorker_;

    static constexpr int MAX_FRAME_QUEUE_SIZE = 20;           // 每个队列最多20个frame
    static constexpr int MAX_BUFFER_SIZE = 5 * 1024 * 1024;   // 5MB后备缓冲区
};

#endif   // CLIENTTHREAD_H