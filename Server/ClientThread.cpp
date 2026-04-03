// ClientThread.cpp
#include "ClientThread.h"

#include <QDateTime>
#include <QDebug>
#include <QThread>

#include "Server.h"

ClientThread::ClientThread(QTcpSocket* socket, const QString& id, Server* server, QObject* parent)
    : QObject(parent), socket_(socket), id_(id), server_(server), active_(true),
      lastHeartbeatTime_(QDateTime::currentMSecsSinceEpoch() / 1000), stopReceiveThread_(false), stopSendThread_(false),
      receiveThread_(nullptr), sendThread_(nullptr)
{
    connect(socket_, &QTcpSocket::readyRead, this, &ClientThread::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &ClientThread::onDisconnected);
    connect(socket_, &QTcpSocket::errorOccurred, this, &ClientThread::onSocketError);
}

ClientThread::~ClientThread()
{
    stop();
}

bool ClientThread::start()
{
    stopReceiveThread_ = false;
    stopSendThread_ = false;

    // recvWorker_ = new QObject();
    // sendWorker_ = new QObject();

    // receiveThread_ = new QThread(this);
    // recvWorker_->moveToThread(receiveThread_);
    // connect(receiveThread_, &QThread::started, recvWorker_, [this]() { processReceiveData(); });
    // receiveThread_->start();

    // sendThread_ = new QThread(this);
    // sendWorker_->moveToThread(sendThread_);
    // connect(sendThread_, &QThread::started, sendWorker_, [this]() { processSendData(); });
    // sendThread_->start();

    return true;
}

void ClientThread::stop()
{
    active_ = false;
    stopReceiveThread_ = true;
    stopSendThread_ = true;

    frameQueueNotEmpty_.wakeAll();
    frameQueueNotFull_.wakeAll();
    sendQueueNotEmpty_.wakeAll();
    sendQueueNotFull_.wakeAll();

    if (receiveThread_) {
        receiveThread_->quit();
        receiveThread_->wait();
        receiveThread_->deleteLater();
        receiveThread_ = nullptr;
    }

    if (sendThread_) {
        sendThread_->quit();
        sendThread_->wait();
        sendThread_->deleteLater();
        sendThread_ = nullptr;
    }

    if (socket_) {
        socket_->abort();
        if (socket_->state() != QAbstractSocket::UnconnectedState) {
            socket_->waitForDisconnected(1000);
        }
        socket_->deleteLater();
        socket_ = nullptr;
    }
}

void ClientThread::onReadyRead()
{
    if (!socket_ || !active_ || stopReceiveThread_) {
        return;
    }

    QByteArray newData = socket_->readAll();

    qDebug() << "收到数据" << newData.size();

    if (newData.isEmpty()) {
        return;
    }

    {
        QMutexLocker locker(&bufferMutex_);

        // 检查总缓冲区大小是否超限
        if (receiveByteBuffer_.size() + newData.size() > MAX_BUFFER_SIZE) {
            qWarning() << "Client" << id_ << "receive buffer full, dropping connection";
            socket_->abort();
            return;
        }

        receiveByteBuffer_.append(newData);
    }
}

void ClientThread::onDisconnected()
{
    active_ = false;
    emit disconnected(id_);
}

void ClientThread::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    if (socket_) {
        qWarning() << "Socket error for client" << id_ << ":" << socket_->errorString();
    }
    active_ = false;
    emit disconnected(id_);
}

void ClientThread::processReceiveData()
{
    while (!stopReceiveThread_) {
        QSharedPointer<FrameBuffer> frame;

        {
            QMutexLocker locker(&bufferMutex_);
            frame = Protocol::ParseBuffer(receiveByteBuffer_);
            if (frame.isNull()) {
                if (!stopReceiveThread_) {
                    locker.unlock();
                    QThread::msleep(1);
                }
                continue;
            }

            // 成功解析出一个frame
            // 注意：Protocol::ParseBuffer会从receiveByteBuffer_中移除已解析的数据
        }

        if (frame) {
            frame->fid = id_;

            if (frame->type == FBT_SER_MSG_HEARTBEAT) {
                updateHeartbeatTime();
                emit heartbeatReceived(id_);
            } else {
                // 放入frame队列
                {
                    QMutexLocker locker(&frameQueueMutex_);

                    // 等待frame队列有空间
                    while (receiveFrameQueue_.size() >= MAX_FRAME_QUEUE_SIZE && !stopReceiveThread_) {
                        frameQueueNotFull_.wait(&frameQueueMutex_, 100);
                    }

                    if (stopReceiveThread_) {
                        break;
                    }

                    receiveFrameQueue_.enqueue(frame);
                    frameQueueNotEmpty_.wakeOne();
                }

                // 通知Server有新的frame到达
                emit dataReceived(id_, frame);
            }
        }
    }
}

bool ClientThread::forwardData(const QSharedPointer<FrameBuffer>& frame)
{
    if (!active_ || !socket_ || socket_->state() != QAbstractSocket::ConnectedState) {
        return false;
    }

    {
        QMutexLocker locker(&sendQueueMutex_);

        // 等待发送队列有空间
        while (sendFrameQueue_.size() >= MAX_FRAME_QUEUE_SIZE && !stopSendThread_) {
            if (!socket_ || socket_->state() != QAbstractSocket::ConnectedState) {
                return false;
            }
            sendQueueNotFull_.wait(&sendQueueMutex_, 100);
        }

        if (stopSendThread_) {
            return false;
        }

        sendFrameQueue_.enqueue(frame);
        sendQueueNotEmpty_.wakeOne();
    }

    return true;
}

void ClientThread::processSendData()
{
    while (!stopSendThread_) {
        QSharedPointer<FrameBuffer> frame;

        {
            QMutexLocker locker(&sendQueueMutex_);

            while (sendFrameQueue_.isEmpty() && !stopSendThread_) {
                sendQueueNotEmpty_.wait(&sendQueueMutex_, 100);
            }

            if (stopSendThread_) {
                break;
            }

            frame = sendFrameQueue_.dequeue();
            sendQueueNotFull_.wakeOne();
        }

        if (frame && socket_ && socket_->state() == QAbstractSocket::ConnectedState) {
            QByteArray data = Protocol::PackBuffer(frame);
            if (data.isEmpty()) {
                qWarning() << "Failed to pack frame for client:" << id_;
                continue;
            }

            qint64 bytesWritten = 0;
            while (bytesWritten < data.size() && !stopSendThread_) {
                qint64 written = socket_->write(data.constData() + bytesWritten, data.size() - bytesWritten);
                if (written < 0) {
                    qWarning() << "Failed to write to socket:" << id_;
                    break;
                }
                bytesWritten += written;
                if (!socket_->waitForBytesWritten(20000)) {
                    qWarning() << "Timeout waiting for bytes written:" << id_;
                    break;
                }
            }
        }
    }
}