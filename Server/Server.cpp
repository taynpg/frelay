#include "Server.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QHostAddress>
#include <QThread>

#include "InfoClient.h"
#include "InfoMsg.h"
#include "InfoPack.hpp"

Server::Server(QObject* parent) : QTcpServer(parent)
{
    monitorTimer_ = new QTimer(this);
    monitorTimer_->setSingleShot(false);
    connect(monitorTimer_, &QTimer::timeout, this, &Server::monitorClients);
    connect(this, &Server::newConnection, this, &Server::onNewConnection);

    // 每5秒清理一次已断开的客户端
    QTimer* cleanupTimer = new QTimer(this);
    cleanupTimer->start(5000);
    connect(cleanupTimer, &QTimer::timeout, this, &Server::cleanupDisconnectedClients);
}

Server::~Server()
{
    stopServer();
}

bool Server::startServer(quint16 port)
{
    if (!listen(QHostAddress::Any, port)) {
        qWarning() << "Server start failed:" << errorString();
        return false;
    }

    qInfo() << "Server started on port" << serverPort();
    monitorTimer_->start(MONITOR_HEART_SPED);
    id_ = QString("0.0.0.0:%1").arg(serverPort());
    return true;
}

void Server::stopServer()
{
    if (monitorTimer_) {
        monitorTimer_->stop();
    }

    close();

    // 先停止所有客户端
    {
        QWriteLocker locker(&rwLock_);
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            auto& clientInfo = it.value();
            if (clientInfo.clientWorker) {
                // 异步停止，避免死锁
                QMetaObject::invokeMethod(clientInfo.clientWorker.data(), "stop", Qt::QueuedConnection);
            }
        }
    }

    // 等待所有线程退出
    int waitCount = 0;
    const int maxWait = 30;   // 最多等待3秒

    while (waitCount < maxWait) {
        bool allStopped = true;
        {
            QReadLocker locker(&rwLock_);
            for (auto& client : clients_) {
                if (client.clientWorker && client.clientWorker->isActive()) {
                    allStopped = false;
                    break;
                }
            }
        }

        if (allStopped)
            break;

        QThread::msleep(100);
        waitCount++;
        QCoreApplication::processEvents();
    }

    // 清理资源
    {
        QWriteLocker locker(&rwLock_);
        for (auto& client : clients_) {
            if (client.workerTh) {
                if (client.workerTh->isRunning()) {
                    client.workerTh->quit();
                    client.workerTh->wait(1000);
                }
                client.workerTh->deleteLater();
            }
        }
        clients_.clear();
    }

    qInfo() << "Server stopped";
}

void Server::onNewConnection()
{
    QTcpSocket* clientSocket = nextPendingConnection();
    if (!clientSocket) {
        qWarning() << "Invalid socket received";
        return;
    }

    // 解除与创建线程的关系。
    clientSocket->setParent(nullptr);

    QHostAddress peerAddress = clientSocket->peerAddress();
    quint32 ipv4 = peerAddress.toIPv4Address();
    QString ipStr = QHostAddress(ipv4).toString();
    QString clientId = QString("%1:%2").arg(ipStr).arg(clientSocket->peerPort());

    // 检查客户端数量限制
    {
        QReadLocker locker(&rwLock_);
        if (clients_.size() >= MAX_CLIENTS) {
            qWarning() << "Client connection refused (max limit reached):" << clientId;
            clientSocket->disconnectFromHost();
            clientSocket->deleteLater();
            return;
        }
    }

    // 发送欢迎消息
    QByteArray welcomeMsg = "Hello from server";
    qint64 bytesWritten = clientSocket->write(welcomeMsg);
    if (bytesWritten != welcomeMsg.size()) {
        qWarning() << "Failed to send welcome message to" << clientId;
    }

    // 创建客户端工作线程 - 保持原构造函数调用方式
    ClientInfo cli;
    cli.workerTh = new QThread();
    cli.clientWorker = QSharedPointer<ClientThread>::create(clientId, nullptr);
    cli.lastHeartbeatCheck = QDateTime::currentSecsSinceEpoch();

    // 移动到工作线程
    cli.clientWorker->moveToThread(cli.workerTh);
    clientSocket->moveToThread(cli.workerTh);

    // 连接信号槽
    connect(cli.clientWorker.get(), &ClientThread::disconnected, this, &Server::onClientDisconnected, Qt::QueuedConnection);
    connect(cli.clientWorker.get(), &ClientThread::dataReceived, this, &Server::onClientDataReceived, Qt::QueuedConnection);
    connect(cli.clientWorker.get(), &ClientThread::heartbeatReceived, this, &Server::onClientHeartbeat, Qt::QueuedConnection);
    connect(cli.clientWorker.get(), &ClientThread::heartbeatTimeout, this, &Server::onClientHeartbeatTimeout,
            Qt::QueuedConnection);

    // 添加到客户端列表
    {
        QWriteLocker locker(&rwLock_);
        if (clients_.contains(clientId)) {
            qWarning() << "Client already exists:" << clientId;
            // 关闭旧的连接
            if (clients_[clientId].clientWorker) {
                QMetaObject::invokeMethod(clients_[clientId].clientWorker.data(), "stop", Qt::QueuedConnection);
            }
        }
        clients_.insert(clientId, cli);
    }

    // 启动线程
    cli.workerTh->start();

    // 在客户端线程中启动 - 保持原代码的阻塞调用方式
    bool started = false;
    QMetaObject::invokeMethod(
        cli.clientWorker.get(),
        [clientSocket, &started, worker = cli.clientWorker.get()]() { started = worker->start(clientSocket); },
        Qt::BlockingQueuedConnection);

    if (!started) {
        qWarning() << "Failed to start client worker:" << clientId;
        {
            QWriteLocker locker(&rwLock_);
            clients_.remove(clientId);
        }

        // 停止线程
        cli.workerTh->quit();
        cli.workerTh->wait();
        cli.workerTh->deleteLater();

        // 清理socket
        clientSocket->deleteLater();

        return;
    }

    // 发送客户端ID
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->type = FBT_SER_MSG_YOURID;
    frame->fid = id_;
    frame->tid = clientId;
    frame->data = clientId.toUtf8();

    cli.clientWorker->queueDataForSending(frame);

    qInfo() << "Client connected:" << clientId << "Total clients:" << clients_.size();
}

void Server::onClientDataReceived(const QString& clientId, const QSharedPointer<FrameBuffer>& frame)
{
    if (!frame) {
        qWarning() << "Received null frame from" << clientId;
        return;
    }

    qDebug() << "Received data from" << clientId << "type:" << frame->type;

    if (frame->type <= 30) {
        // 服务请求
        switch (frame->type) {
        case FBT_SER_MSG_HEARTBEAT:
            onClientHeartbeat(clientId);
            break;

        case FBT_SER_MSG_ASKCLIENTS: {
            QByteArray clientList = getClients();
            auto replyFrame = QSharedPointer<FrameBuffer>::create();
            replyFrame->type = FBT_SER_MSG_ASKCLIENTS;
            replyFrame->fid = id_;
            replyFrame->tid = clientId;
            replyFrame->data = clientList;
            forwardDataToClient(clientId, replyFrame);
            break;
        }

        case FBT_SER_MSG_JUDGE_OTHER_ALIVE: {
            bool targetAlive = false;
            {
                QReadLocker locker(&rwLock_);
                targetAlive = clients_.contains(frame->tid);
            }

            if (!targetAlive) {
                auto replyFrame = QSharedPointer<FrameBuffer>::create();
                replyFrame->type = FBT_SER_MSG_OFFLINE;
                replyFrame->fid = frame->tid;
                replyFrame->tid = clientId;
                forwardDataToClient(clientId, replyFrame);
            }
            break;
        }

        default:
            qWarning() << "Unknown request type:" << frame->type << "from" << frame->fid;
            break;
        }
    } else {
        // 数据转发
        forwardDataToClient(frame->fid, frame);
    }
}

void Server::onClientHeartbeat(const QString& clientId)
{
    QSharedPointer<ClientThread> client = findClientThread(clientId);

    if (client) {
        // 在客户端线程中更新心跳时间
        QMetaObject::invokeMethod(client.data(), "updateHeartbeatTime", Qt::QueuedConnection);
    }
}

void Server::monitorClients()
{
    qint64 now = QDateTime::currentSecsSinceEpoch();
    std::vector<QString> prepareRemove;

    {
        QReadLocker locker(&rwLock_);

        // 遍历所有客户端，但异步检查状态
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            const QString& clientId = it.key();
            ClientInfo& clientInfo = it.value();

            // 检查是否需要心跳检查
            if (now - clientInfo.lastHeartbeatCheck < 5) {   // 5秒检查一次
                continue;
            }

            clientInfo.lastHeartbeatCheck = now;

            // 异步检查客户端状态
            checkClientAliveAsync(clientId);
        }
    }
}

void Server::checkClientAliveAsync(const QString& clientId)
{
    QSharedPointer<ClientThread> client = findClientThread(clientId);
    if (!client) {
        return;
    }

    // 在客户端线程中检查活跃状态
    QMetaObject::invokeMethod(
        client.data(),
        [client]() {
            bool isActive = client->isActive();
            qint64 lastHeartbeat = client->getLastHeartbeatTime();
            qint64 now = QDateTime::currentSecsSinceEpoch();

            if (isActive && (now - lastHeartbeat) > NO_HEATBEAT_TIMEOUT) {
                emit client->heartbeatTimeout(client->getId(), lastHeartbeat);
            }
        },
        Qt::QueuedConnection);
}

void Server::onClientHeartbeatTimeout(const QString& clientId, qint64 lastTime)
{
    qInfo() << "Client" << clientId << "heartbeat timeout, last heartbeat:" << lastTime;

    // 从客户端线程发出的信号，在主线程中断开连接
    QMetaObject::invokeMethod(this, [this, clientId]() { onClientDisconnected(clientId); }, Qt::QueuedConnection);
}

void Server::onClientDisconnected(const QString& clientId)
{
    qDebug() << "Processing disconnect for client:" << clientId;

    QSharedPointer<ClientThread> clientWorker;
    QThread* workerThread = nullptr;

    {
        QWriteLocker locker(&rwLock_);
        if (clients_.contains(clientId)) {
            auto& cli = clients_[clientId];
            clientWorker = cli.clientWorker;
            workerThread = cli.workerTh;
            clients_.remove(clientId);
            qDebug() << "Removed client from map:" << clientId;
        } else {
            qDebug() << "Client not found in map:" << clientId;
            return;
        }
    }

    // 在客户端线程中停止
    if (clientWorker) {
        QMetaObject::invokeMethod(clientWorker.data(), "stop", Qt::BlockingQueuedConnection);
    }

    // 停止线程
    if (workerThread) {
        if (workerThread->isRunning()) {
            workerThread->quit();
            if (!workerThread->wait(2000)) {
                qWarning() << "Failed to stop thread for client:" << clientId;
                workerThread->terminate();
                workerThread->wait();
            }
        }
        workerThread->deleteLater();
    }

    qInfo() << "Client disconnected:" << clientId;
}

QByteArray Server::getClients()
{
    InfoClientVec infoClients;

    {
        QReadLocker locker(&rwLock_);
        for (auto& c : clients_) {
            InfoClient infoClient;
            infoClient.id = c.clientWorker->getId();
            infoClients.vec.append(infoClient);
        }
    }

    auto ret = infoPack<InfoClientVec>(infoClients);
    return ret;
}

bool Server::forwardDataToClient(const QString& fromId, QSharedPointer<FrameBuffer> frame)
{
    if (!frame) {
        qWarning() << "Cannot forward null frame from" << fromId;
        return false;
    }

    QSharedPointer<ClientThread> targetClient = findClientThread(frame->tid);

    if (targetClient && targetClient->isConnected()) {
        bool success = targetClient->queueDataForSending(frame);
        if (!success) {
            qWarning() << "Failed to forward data from" << fromId << "to" << frame->tid << "(queue full)";

            // 通知发送方转发失败
            QSharedPointer<ClientThread> fromClient = findClientThread(fromId);

            if (fromClient && fromClient->isConnected()) {
                auto errorFrame = QSharedPointer<FrameBuffer>::create();
                errorFrame->type = FBT_SER_MSG_FORWARD_FAILED;
                errorFrame->fid = id_;
                errorFrame->tid = fromId;
                errorFrame->data = QString("Target client %1 queue full").arg(frame->tid).toUtf8();
                fromClient->queueDataForSending(errorFrame);
            }
        }
        return success;
    } else {
        // 目标不存在或未连接
        qWarning() << "Target client not found or disconnected:" << frame->tid;

        QSharedPointer<ClientThread> fromClient = findClientThread(fromId);

        if (fromClient && fromClient->isConnected()) {
            auto errorFrame = QSharedPointer<FrameBuffer>::create();
            errorFrame->type = FBT_SER_MSG_FORWARD_FAILED;
            errorFrame->fid = id_;
            errorFrame->tid = fromId;
            errorFrame->data = QString("Target client %1 not found").arg(frame->tid).toUtf8();
            fromClient->queueDataForSending(errorFrame);
        }
        return false;
    }
}

QSharedPointer<ClientThread> Server::findClientThread(const QString& clientId) const
{
    QReadLocker locker(&rwLock_);
    if (clients_.contains(clientId)) {
        return clients_.value(clientId).clientWorker;
    }
    return nullptr;
}

void Server::cleanupDisconnectedClients()
{
    // 这个方法会在定时器中调用，清理那些已标记为断开但还未从列表中移除的客户端
    {
        QWriteLocker locker(&rwLock_);
        auto it = clients_.begin();
        while (it != clients_.end()) {
            if (!it.value().clientWorker || !it.value().clientWorker->isActive()) {
                qDebug() << "Cleaning up disconnected client:" << it.key();
                it = clients_.erase(it);
            } else {
                ++it;
            }
        }
    }
}