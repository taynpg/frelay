// ClientThread.h
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
    explicit ClientThread(QTcpSocket* socket, const QString& id, Server* server, QObject* parent = nullptr);
    ~ClientThread();

    bool start();
    void stop();
    bool forwardData(const QSharedPointer<FrameBuffer>& frame);
    QString getId() const
    {
        return id_;
    }
    qint64 getLastHeartbeatTime() const
    {
        return lastHeartbeatTime_;
    }
    void updateHeartbeatTime()
    {
        lastHeartbeatTime_ = QDateTime::currentMSecsSinceEpoch() / 1000;
    }
    bool isActive() const
    {
        return active_;
    }
    bool isConnected() const
    {
        return socket_ && socket_->state() == QAbstractSocket::ConnectedState;
    }

signals:
    void disconnected(const QString& clientId);
    void dataReceived(const QString& clientId, const QSharedPointer<FrameBuffer>& frame);
    void heartbeatReceived(const QString& clientId);

public slots:
    void onReadyRead();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);

private:
    void processReceiveData();
    void processSendData();

    QTcpSocket* socket_;
    QString id_;
    Server* server_;
    bool active_;
    qint64 lastHeartbeatTime_;

    QByteArray receiveByteBuffer_;
    QMutex bufferMutex_;

    QQueue<QSharedPointer<FrameBuffer>> receiveFrameQueue_;
    QMutex frameQueueMutex_;
    QWaitCondition frameQueueNotEmpty_;
    QWaitCondition frameQueueNotFull_;
    bool stopReceiveThread_;
    QThread* receiveThread_;

    QQueue<QSharedPointer<FrameBuffer>> sendFrameQueue_;
    QMutex sendQueueMutex_;
    QWaitCondition sendQueueNotEmpty_;
    QWaitCondition sendQueueNotFull_;
    bool stopSendThread_;
    QThread* sendThread_;

    QObject* recvWorker_{};
    QObject* sendWorker_{};

    static constexpr int MAX_FRAME_QUEUE_SIZE = 20;           // 每个队列最多20个frame
    static constexpr int MAX_BUFFER_SIZE = 5 * 1024 * 1024;   // 5MB后备缓冲区
};

#endif   // CLIENTTHREAD_H