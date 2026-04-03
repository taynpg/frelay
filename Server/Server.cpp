// Server.cpp
#include "Server.h"

#include <QDateTime>
#include <QDebug>
#include <QHostAddress>

#include "InfoClient.h"
#include "InfoMsg.h"
#include "InfoPack.hpp"

Server::Server(QObject* parent) : QTcpServer(parent)
{
    monitorTimer_ = new QTimer(this);
    connect(monitorTimer_, &QTimer::timeout, this, &Server::monitorClients);
    connect(this, &Server::newConnection, this, &Server::onNewConnection);
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
    monitorTimer_->stop();
    close();

    QWriteLocker locker(&rwLock_);
    for (auto& client : clients_) {
        client.clientWorker->stop();
    }
    clients_.clear();
}

void Server::onNewConnection()
{
    QTcpSocket* clientSocket = nextPendingConnection();

    QHostAddress peerAddress = clientSocket->peerAddress();
    quint32 ipv4 = peerAddress.toIPv4Address();
    QString ipStr = QHostAddress(ipv4).toString();
    QString clientId = QString("%1:%2").arg(ipStr).arg(clientSocket->peerPort());

    if (clients_.size() >= 100) {
        qWarning() << "Client connection refused (max limit reached):" << clientId;
        clientSocket->disconnectFromHost();
        clientSocket->deleteLater();
        return;
    }

    auto d = clientSocket->write("Hello1");
    qDebug() << d;

    ClientInfo cli;
    cli.workerTh = new QThread();
    cli.clientWorker = QSharedPointer<ClientThread>::create(clientSocket, clientId, nullptr);
    cli.clientWorker->moveToThread(cli.workerTh);
    clientSocket->moveToThread(cli.workerTh);

    connect(cli.clientWorker.get(), &ClientThread::disconnected, this, &Server::onClientDisconnected);
    connect(cli.clientWorker.get(), &ClientThread::dataReceived, this, &Server::onClientDataReceived);
    connect(cli.clientWorker.get(), &ClientThread::heartbeatReceived, this, &Server::onClientHeartbeat);

    cli.workerTh->start();

    {
        QWriteLocker locker(&rwLock_);
        clients_.insert(clientId, cli);
    }

    if (!cli.clientWorker->start()) {
        qWarning() << "Failed to start client thread:" << clientId;
        {
            QWriteLocker locker(&rwLock_);
            clients_.remove(clientId);
        }
        clientSocket->deleteLater();
        return;
    }

    // 发送客户端ID
    // auto frame = QSharedPointer<FrameBuffer>::create();
    // frame->type = FBT_SER_MSG_YOURID;
    // frame->fid = id_;
    // frame->tid = clientId;
    // frame->data = clientId.toUtf8();
    // cli.clientWorker->forwardData(frame);

    qInfo() << "Client connected:" << clientId;
}

void Server::onClientDisconnected(const QString& clientId)
{
    {
        QWriteLocker locker(&rwLock_);
        if (clients_.contains(clientId)) {
            clients_[clientId].clientWorker->stop();
            clients_.remove(clientId);
        }
    }

    qInfo() << "Client disconnected:" << clientId;
}

void Server::onClientDataReceived(const QString& clientId, const QSharedPointer<FrameBuffer>& frame)
{
    if (frame->type <= 30) {
        // 服务请求
        switch (frame->type) {
        case FBT_SER_MSG_HEARTBEAT:
            onClientHeartbeat(frame->fid);
            break;

        case FBT_SER_MSG_ASKCLIENTS: {
            QByteArray clientList = getClients();
            auto replyFrame = QSharedPointer<FrameBuffer>::create();
            replyFrame->type = FBT_SER_MSG_ASKCLIENTS;
            replyFrame->fid = id_;
            replyFrame->tid = frame->fid;
            replyFrame->data = clientList;
            forwardDataToClient(frame->fid, replyFrame);
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
                replyFrame->tid = frame->fid;
                forwardDataToClient(frame->fid, replyFrame);
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
    QSharedPointer<ClientThread> client;
    {
        QReadLocker locker(&rwLock_);
        if (clients_.contains(clientId)) {
            client = clients_.value(clientId).clientWorker;
        }
    }

    if (client) {
        client->updateHeartbeatTime();
    }
}

void Server::monitorClients()
{
    qint64 now = QDateTime::currentMSecsSinceEpoch() / 1000;
    std::vector<QString> prepareRemove;

    {
        QReadLocker locker(&rwLock_);
        for (auto it = clients_.begin(); it != clients_.end(); ++it) {
            auto& client = it.value();
            if (client.clientWorker->isActive() && (now - client.clientWorker->getLastHeartbeatTime()) > NO_HEATBEAT_TIMEOUT) {
                prepareRemove.push_back(it.key());
            }
        }
    }

    for (const auto& clientId : prepareRemove) {
        qInfo() << "Removing inactive client:" << clientId;
        {
            QWriteLocker locker(&rwLock_);
            if (clients_.contains(clientId)) {
                clients_[clientId].clientWorker->stop();
                clients_.remove(clientId);
            }
        }
    }
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
    QSharedPointer<ClientThread> targetClient;
    {
        QReadLocker locker(&rwLock_);
        if (clients_.contains(frame->tid)) {
            targetClient = clients_.value(frame->tid).clientWorker;
        }
    }

    if (targetClient && targetClient->isConnected()) {
        bool success = targetClient->forwardData(frame);
        if (!success) {
            qWarning() << "Failed to forward data from" << fromId << "to" << frame->tid << "(queue full)";

            // 通知发送方转发失败
            QSharedPointer<ClientThread> fromClient;
            {
                QReadLocker locker(&rwLock_);
                if (clients_.contains(fromId)) {
                    fromClient = clients_.value(fromId).clientWorker;
                }
            }

            if (fromClient && fromClient->isConnected()) {
                auto errorFrame = QSharedPointer<FrameBuffer>::create();
                errorFrame->type = FBT_SER_MSG_FORWARD_FAILED;
                errorFrame->fid = id_;
                errorFrame->tid = fromId;
                errorFrame->data = QString("Target client %1 queue full").arg(frame->tid).toUtf8();
                fromClient->forwardData(errorFrame);
            }
        }
        return success;
    } else {
        // 目标不存在或未连接
        QSharedPointer<ClientThread> fromClient;
        {
            QReadLocker locker(&rwLock_);
            if (clients_.contains(fromId)) {
                fromClient = clients_.value(fromId).clientWorker;
            }
        }

        if (fromClient && fromClient->isConnected()) {
            auto errorFrame = QSharedPointer<FrameBuffer>::create();
            errorFrame->type = FBT_SER_MSG_FORWARD_FAILED;
            errorFrame->fid = id_;
            errorFrame->tid = fromId;
            errorFrame->data = QString("Target client %1 not found").arg(frame->tid).toUtf8();
            fromClient->forwardData(errorFrame);
        }
        return false;
    }
}