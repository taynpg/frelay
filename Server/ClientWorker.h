#ifndef CLIENT_WORKER_H
#define CLIENT_WORKER_H

#include <QDateTime>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QTcpSocket>
#include <QThread>
#include <QWaitCondition>

#include "Protocol.h"

class ClientWorker : public QObject
{
    Q_OBJECT

public:
    explicit ClientWorker(const QString& id, QObject* parent);

public:
    bool InitSocket(QTcpSocket* sock);
    QString GetID();

signals:
    void sigHaveFrame(const QString& id, QSharedPointer<FrameBuffer>);

public slots:
    void onDataReadyIn();
    void onDataReadyOutIn(QSharedPointer<FrameBuffer> frame);
    bool Send(QSharedPointer<FrameBuffer> frame);

private:
    void handleRecvTh();
    void handleSendTH();
    bool syncSend(QSharedPointer<FrameBuffer> frame);

private:
    QString id_;
    QByteArray sourceBuffer_;
    QTcpSocket* socket_;

    QObject* recvWorker_;
    QObject* sendWorker_;

    QThread* recvTh_;
    QThread* useTh_;

    QQueue<QSharedPointer<FrameBuffer>> recvBuf_;
    QQueue<QSharedPointer<FrameBuffer>> sendBuf_;

    QWaitCondition cvRecvIn_;
    QWaitCondition cvRecvOut_;
    QWaitCondition cvSendIn_;
    QWaitCondition cvSendOut_;

    QMutex sockMut_;
    QMutex recvMut_;
    QMutex sendMut_;

    bool isRun_{};

    static constexpr int MAX_FRAME_QUEUE_SIZE = 20;           // 每个队列最多20个frame
    static constexpr int MAX_BUFFER_SIZE = 5 * 1024 * 1024;   // 5MB后备缓冲区
};

#endif