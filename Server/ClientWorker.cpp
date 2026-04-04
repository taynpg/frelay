#include "ClientWorker.h"

constexpr int CV_WAIT_TIME = 100;

ClientWorker::ClientWorker(const QString& id, QObject* parent) : id_(id), QObject(parent)
{
}

bool ClientWorker::InitSocket(QTcpSocket* sock)
{
    QMutexLocker locker(&sockMut_);

    socket_ = sock;
    socket_->setParent(this);

    connect(socket_, &QTcpSocket::readyRead, this, &ClientWorker::onDataReadyIn, Qt::QueuedConnection);
    // connect(socket_, &QTcpSocket::disconnected, this, &ClientWorker::onDisconnected, Qt::QueuedConnection);
    // connect(socket_, &QTcpSocket::errorOccurred, this, &ClientWorker::onSocketError, Qt::QueuedConnection);

    isRun_ = true;

    recvWorker_ = new QObject();
    sendWorker_ = new QObject();

    // 创建接收线程
    recvTh_ = new QThread(this);
    recvWorker_->moveToThread(recvTh_);
    connect(recvTh_, &QThread::started, recvWorker_, [this]() { handleRecvTh(); });
    connect(recvTh_, &QThread::finished, recvWorker_, &QObject::deleteLater);
    recvTh_->start();

    // 创建发送线程
    useTh_ = new QThread(this);
    sendWorker_->moveToThread(useTh_);
    connect(useTh_, &QThread::started, sendWorker_, [this]() { handleSendTH(); });
    connect(useTh_, &QThread::finished, sendWorker_, &QObject::deleteLater);
    useTh_->start();

    qDebug() << "ClientThread started for" << id_;
    return true;
}

QString ClientWorker::GetID()
{
    return id_;
}

void ClientWorker::onDataReadyIn()
{
    auto newer = socket_->readAll();
    sourceBuffer_.append(newer);

    if (sourceBuffer_.size() > MAX_BUFFER_SIZE) {
        // 主动向整体逻辑请求断开
    }

    while (true) {
        QMutexLocker locker(&recvMut_);
        while (recvBuf_.size() >= MAX_FRAME_QUEUE_SIZE) {
            if (!isRun_) {
                return;
            }
            cvRecvIn_.wait(&recvMut_, CV_WAIT_TIME);
        }
        if (!isRun_) {
            return;
        }
        auto frame = Protocol::ParseBuffer(sourceBuffer_);
        if (frame.isNull()) {
            return;
        }
        frame->fid = id_;
        recvBuf_.enqueue(frame);
        cvRecvOut_.wakeOne();
    }
}

void ClientWorker::onDataReadyOutIn(QSharedPointer<FrameBuffer> frame)
{
    QMutexLocker locker(&sendMut_);
    while (sendBuf_.size() >= MAX_FRAME_QUEUE_SIZE) {
        if (!isRun_) {
            return;
        }
        cvSendIn_.wait(&sendMut_, CV_WAIT_TIME);
    }
    if (!isRun_) {
        return;
    }
    sendBuf_.enqueue(frame);
    cvSendOut_.wakeOne();
}

void ClientWorker::handleRecvTh()
{
    while (isRun_) {

        QSharedPointer<FrameBuffer> frame;

        {
            QMutexLocker locker(&recvMut_);
            while (recvBuf_.isEmpty()) {
                if (!isRun_) {
                    return;
                }
                cvRecvOut_.wait(&recvMut_, CV_WAIT_TIME);
            }
            if (!isRun_) {
                return;
            }
            frame = recvBuf_.dequeue();
            cvRecvIn_.wakeOne();
        }

        // 通知Server处理数据
        emit sigHaveFrame(id_, frame);
    }
}

void ClientWorker::handleSendTH()
{
    while (isRun_) {

        QSharedPointer<FrameBuffer> frame;

        {
            QMutexLocker locker(&sendMut_);
            while (sendBuf_.isEmpty()) {
                if (!isRun_) {
                    return;
                }
                cvSendOut_.wait(&sendMut_, CV_WAIT_TIME);
            }
            if (!isRun_) {
                return;
            }
            frame = sendBuf_.dequeue();
            cvSendIn_.wakeOne();
        }

        // 发送数据
        syncSend(frame);
    }
}

bool ClientWorker::syncSend(QSharedPointer<FrameBuffer> frame)
{
    bool result = false;
    bool success = QMetaObject::invokeMethod(this, "Send", Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool, result),
                                             Q_ARG(QSharedPointer<FrameBuffer>, frame));
    if (!success) {
        return false;
    }
    return result;
}

bool ClientWorker::Send(QSharedPointer<FrameBuffer> frame)
{
    if (frame == nullptr) {
        return false;
    }
    auto data = Protocol::PackBuffer(frame);
    if (data.size() == 0) {
        return false;
    }
    if (socket_->state() != QAbstractSocket::ConnectedState) {
        qCritical() << QString("%1 未连接...").arg(id_);
        return false;
    }
    qint64 bytesWritten = -1;
    {
        QMutexLocker locker(&sockMut_);
        bytesWritten = socket_->write(data.constData(), data.size());
    }
    // 这里 1000 * 300 应对网络非常慢的情况。
    if (bytesWritten == -1 || !socket_->waitForBytesWritten(1000 * 300)) {
        qCritical() << QString("发送数据失败:[%1], written:[%2]").arg(socket_->errorString()).arg(bytesWritten);
        return false;
    }
    return true;
}