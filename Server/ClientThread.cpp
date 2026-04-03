#include "ClientThread.h"

#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QTimer>

#include "Server.h"

constexpr int CV_WAIT_TIME = 1;

ClientThread::ClientThread(const QString& id, Server* server, QObject* parent)
    : QObject(parent), socket_(nullptr), id_(id), server_(server), active_(true),
      lastHeartbeatTime_(QDateTime::currentSecsSinceEpoch()), stopReceiveThread_(false), stopSendThread_(false),
      receiveThread_(nullptr), sendThread_(nullptr), recvWorker_(nullptr), sendWorker_(nullptr)
{
    // 构造函数在主线程中执行，socket的连接在start()中设置
}

ClientThread::~ClientThread()
{
    stop();
}

bool ClientThread::start(QTcpSocket* socket)
{
    if (!socket) {
        qWarning() << "ClientThread: Invalid socket provided";
        return false;
    }

             // 设置socket
    {
        QMutexLocker locker(&socketMutex_);
        socket_ = socket;
        socket_->setParent(this);
    }

             // 初始化socket连接
    initSocketConnections();

             // 初始化工作线程
    stopReceiveThread_ = false;
    stopSendThread_ = false;

    recvWorker_ = new QObject();
    sendWorker_ = new QObject();

             // 创建接收线程
    receiveThread_ = new QThread(this);
    recvWorker_->moveToThread(receiveThread_);
    connect(receiveThread_, &QThread::started, recvWorker_, [this]() { processReceiveData(); });
    connect(receiveThread_, &QThread::finished, recvWorker_, &QObject::deleteLater);
    receiveThread_->start();

             // 创建发送线程
    sendThread_ = new QThread(this);
    sendWorker_->moveToThread(sendThread_);
    connect(sendThread_, &QThread::started, sendWorker_, [this]() { processSendData(); });
    connect(sendThread_, &QThread::finished, sendWorker_, &QObject::deleteLater);
    sendThread_->start();

    qDebug() << "ClientThread started for" << id_;
    return true;
}

void ClientThread::initSocketConnections()
{
    QMutexLocker locker(&socketMutex_);
    if (!socket_) {
        return;
    }

             // 这些连接会自动在正确的线程中执行
    connect(socket_, &QTcpSocket::readyRead, this, &ClientThread::onReadyRead, Qt::QueuedConnection);
    connect(socket_, &QTcpSocket::disconnected, this, &ClientThread::onDisconnected, Qt::QueuedConnection);
    connect(socket_, &QTcpSocket::errorOccurred, this, &ClientThread::onSocketError, Qt::QueuedConnection);
}

void ClientThread::stop()
{
    active_ = false;

    {
        QMutexLocker locker(&frameQueueMutex_);
        stopReceiveThread_ = true;
        frameQueueNotEmpty_.wakeAll();
        frameQueueNotFull_.wakeAll();
    }

    {
        QMutexLocker locker(&sendQueueMutex_);
        stopSendThread_ = true;
        sendQueueNotEmpty_.wakeAll();
        sendQueueNotFull_.wakeAll();
    }

             // 异步执行清理
    QMetaObject::invokeMethod(this, [this]() {
        cleanupThreads();
    }, Qt::QueuedConnection);
}

void ClientThread::cleanupThreads()
{
    // 停止接收线程
    if (receiveThread_) {
        if (receiveThread_->isRunning()) {
            receiveThread_->quit();
            if (!receiveThread_->wait(1000)) {
                qWarning() << "Failed to stop receive thread for client:" << id_;
                receiveThread_->terminate();
                receiveThread_->wait();
            }
        }
        receiveThread_->deleteLater();
        receiveThread_ = nullptr;
    }

             // 停止发送线程
    if (sendThread_) {
        if (sendThread_->isRunning()) {
            sendThread_->quit();
            if (!sendThread_->wait(1000)) {
                qWarning() << "Failed to stop send thread for client:" << id_;
                sendThread_->terminate();
                sendThread_->wait();
            }
        }
        sendThread_->deleteLater();
        sendThread_ = nullptr;
    }

             // 清理socket
    {
        QMutexLocker locker(&socketMutex_);
        if (socket_) {
            if (socket_->state() != QAbstractSocket::UnconnectedState) {
                socket_->abort();
            }
            socket_->deleteLater();
            socket_ = nullptr;
        }
    }

    recvWorker_ = nullptr;
    sendWorker_ = nullptr;

    qDebug() << "ClientThread stopped for" << id_;
}

void ClientThread::onReadyRead()
{
    if (!active_) {
        return;
    }

    {
        QMutexLocker locker(&socketMutex_);
        if (!socket_ || !socket_->isValid()) {
            return;
        }
    }

    QByteArray newData;
    {
        QMutexLocker locker(&socketMutex_);
        if (socket_ && socket_->bytesAvailable() > 0) {
            newData = socket_->readAll();
        }
    }

    if (newData.isEmpty()) {
        return;
    }

    {
        QMutexLocker locker(&bufferMutex_);

                 // 检查总缓冲区大小是否超限
        if (receiveByteBuffer_.size() + newData.size() > MAX_BUFFER_SIZE) {
            qWarning() << "Client" << id_ << "receive buffer full, dropping connection";
            {
                QMutexLocker socketLocker(&socketMutex_);
                if (socket_) {
                    socket_->abort();
                }
            }
            return;
        }

        receiveByteBuffer_.append(newData);
    }
}

void ClientThread::updateHeartbeatTime()
{
    lastHeartbeatTime_ = QDateTime::currentSecsSinceEpoch();
}

void ClientThread::onDisconnected()
{
    if (!active_) {
        return;
    }

    active_ = false;
    qDebug() << "Client" << id_ << "socket disconnected";
    emit disconnected(id_);
}

void ClientThread::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);

    if (!active_) {
        return;
    }

    {
        QMutexLocker locker(&socketMutex_);
        if (socket_) {
            qWarning() << "Socket error for client" << id_ << ":" << socket_->errorString();
        }
    }

    active_ = false;
    qDebug() << "Client" << id_ << "socket error, disconnecting";
    emit disconnected(id_);
}

bool ClientThread::syncSendFrame(const QSharedPointer<FrameBuffer>& frame)
{
    if (!frame) {
        return false;
    }

    bool result = false;
    QMetaObject::invokeMethod(this, "directSend", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result),
                              Q_ARG(QSharedPointer<FrameBuffer>, frame));

    return result;
}

bool ClientThread::directSend(QSharedPointer<FrameBuffer> frame)
{
    if (!frame) {
        return false;
    }
    if (!active_) {
        return false;
    }

    QTcpSocket* socketCopy = nullptr;
    {
        QMutexLocker locker(&socketMutex_);
        if (!socket_ || socket_->state() != QAbstractSocket::ConnectedState) {
            return false;
        }
        socketCopy = socket_;
    }

    QByteArray data = Protocol::PackBuffer(frame);
    if (data.isEmpty()) {
        return false;
    }

    qint64 totalWritten = 0;
    const int maxRetry = 3;
    int retryCount = 0;

    while (totalWritten < data.size() && retryCount < maxRetry) {
        qint64 written = 0;
        {
            QMutexLocker locker(&socketMutex_);
            if (!socketCopy || socketCopy->state() != QAbstractSocket::ConnectedState) {
                return false;
            }
            written = socketCopy->write(data.constData() + totalWritten, data.size() - totalWritten);
        }

        if (written < 0) {
            {
                QMutexLocker locker(&socketMutex_);
                if (socketCopy && socketCopy->error() == QAbstractSocket::TemporaryError && retryCount < maxRetry) {
                    retryCount++;
                    QThread::msleep(10);
                    continue;
                }
            }
            return false;
        }

        totalWritten += written;
        retryCount = 0;

                 // 等待数据写入
        {
            QMutexLocker locker(&socketMutex_);
            if (socketCopy && !socketCopy->waitForBytesWritten(10000)) {
                if (socketCopy->error() == QAbstractSocket::TemporaryError && retryCount < maxRetry) {
                    retryCount++;
                    continue;
                }
                return false;
            }
        }
    }

    return totalWritten == data.size();
}

void ClientThread::processReceiveData()
{
    while (true) {
        if (stopReceiveThread_ || !active_) {
            break;
        }

        QSharedPointer<FrameBuffer> frame;

        {
            QMutexLocker locker(&bufferMutex_);
            frame = Protocol::ParseBuffer(receiveByteBuffer_);
        }

        if (frame.isNull()) {
            // 没有完整的数据包，等待
            QThread::msleep(1);
            continue;
        }

        frame->fid = id_;

        if (frame->type == FBT_SER_MSG_HEARTBEAT) {
            updateHeartbeatTime();
        } else {
            // 放入frame队列
            {
                QMutexLocker locker(&frameQueueMutex_);

                         // 等待frame队列有空间
                while (receiveFrameQueue_.size() >= MAX_FRAME_QUEUE_SIZE) {
                    if (stopReceiveThread_ || !active_) {
                        return;
                    }
                    frameQueueNotFull_.wait(&frameQueueMutex_, CV_WAIT_TIME);
                }

                if (stopReceiveThread_ || !active_) {
                    break;
                }
                receiveFrameQueue_.enqueue(frame);
                qDebug() << "当前receiveFrameQueue_.Size()" << receiveFrameQueue_.size();
                frameQueueNotEmpty_.wakeOne();
            }

                     // 通知Server有新的frame到达
            emit dataReceived(id_, frame);
        }
    }
}

bool ClientThread::queueDataForSending(const QSharedPointer<FrameBuffer>& frame)
{
    if (!frame) {
        return false;
    }

    if (!active_ || stopSendThread_) {
        return false;
    }

    {
        QMutexLocker locker(&socketMutex_);
        if (!socket_ || socket_->state() != QAbstractSocket::ConnectedState) {
            return false;
        }
    }

    {
        QMutexLocker locker(&sendQueueMutex_);

                 // 等待发送队列有空间
        while (sendFrameQueue_.size() >= MAX_FRAME_QUEUE_SIZE) {
            if (!active_ || stopSendThread_) {
                return false;
            }

            {
                QMutexLocker socketLocker(&socketMutex_);
                if (!socket_ || socket_->state() != QAbstractSocket::ConnectedState) {
                    return false;
                }
            }

            sendQueueNotFull_.wait(&sendQueueMutex_, CV_WAIT_TIME);
        }

        if (!active_ || stopSendThread_) {
            return false;
        }

        sendFrameQueue_.enqueue(frame);
        sendQueueNotEmpty_.wakeOne();
    }

    return true;
}

QString ClientThread::getId() const
{
    return id_;
}

qint64 ClientThread::getLastHeartbeatTime() const
{
    return lastHeartbeatTime_;
}

bool ClientThread::isActive() const
{
    return active_ && !stopReceiveThread_ && !stopSendThread_;
}

bool ClientThread::isConnected() const
{
    QMutexLocker locker(&socketMutex_);
    return socket_ && socket_->state() == QAbstractSocket::ConnectedState;
}

void ClientThread::processSendData()
{
    while (true) {
        if (stopSendThread_ || !active_) {
            break;
        }

        QSharedPointer<FrameBuffer> frame;

        {
            QMutexLocker locker(&sendQueueMutex_);

                     // 等待发送队列有数据
            while (sendFrameQueue_.isEmpty()) {
                if (stopSendThread_ || !active_) {
                    return;
                }
                sendQueueNotEmpty_.wait(&sendQueueMutex_, CV_WAIT_TIME);
            }

            if (stopSendThread_ || !active_) {
                break;
            }

            frame = sendFrameQueue_.dequeue();
            sendQueueNotFull_.wakeOne();
        }

        if (frame) {
            bool success = directSend(frame);
            if (!success) {
                qWarning() << "Failed to send frame for client:" << frame->tid;

                         // 发送失败，尝试断开连接
                {
                    QMutexLocker socketLocker(&socketMutex_);
                    if (socket_) {
                        socket_->abort();
                    }
                }

                active_ = false;
                emit disconnected(id_);
                break;
            }
        }
    }
}