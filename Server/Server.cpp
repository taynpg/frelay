#include "Server.h"

#include <InfoClient.h>
#include <InfoPack.hpp>

Server::Server(QObject* parent) : QTcpServer(parent)
{
    connect(this, &Server::newConnection, this, &Server::onNewConnection);
}

Server::~Server()
{
}

bool Server::startServer(quint16 port)
{
    if (!listen(QHostAddress::Any, port)) {
        qWarning() << "Server start failed:" << errorString();
        return false;
    }

    qInfo() << "Server started on port" << serverPort();
    id_ = QString("0.0.0.0:%1").arg(serverPort());
    return true;
}

void Server::handleClientFrame(const QString& id, QSharedPointer<FrameBuffer> frame)
{
    qDebug() << "Received data from" << id << "type:" << frame->type;
    if (frame->type <= 30) {
        switch (frame->type) {
        case FBT_SER_MSG_HEARTBEAT: {
            QReadLocker locker(&rwLock_);
            if (clients_.contains(id)) {
                auto& info = clients_[id];
                info->lastHeart = QDateTime::currentSecsSinceEpoch();
            }
            break;
        }
        case FBT_SER_MSG_ASKCLIENTS: {
            QByteArray clientList = getClients();
            auto replyFrame = QSharedPointer<FrameBuffer>::create();
            replyFrame->type = FBT_SER_MSG_ASKCLIENTS;
            replyFrame->fid = id_;
            replyFrame->tid = id;
            replyFrame->data = clientList;
            dataDispatcher(id, replyFrame);
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
                replyFrame->tid = id;
                dataDispatcher(id, replyFrame);
            }
            break;
        }
        default:
            qWarning() << "Unknown request type:" << frame->type << "from" << frame->fid;
            break;
        }
    }
    else {
        dataDispatcher(frame->tid, frame);
    }
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
        if (clients_.contains(clientId)) {
            qWarning() << "Client already exists:" << clientId;
            return;
        }
    }

    // 创建客户端相关资源
    auto info = QSharedPointer<ClientInfo>::create();
    info->workerTh = new QThread();
    info->worker = QSharedPointer<ClientWorker>::create(clientId, nullptr);
    info->lastHeart = QDateTime::currentSecsSinceEpoch();
    info->worker->moveToThread(info->workerTh);
    clientSocket->moveToThread(info->workerTh);

    connect(info->worker.get(), &ClientWorker::sigHaveFrame, this, &Server::handleClientFrame, Qt::QueuedConnection);

    {
        QWriteLocker locker(&rwLock_);
        clients_.insert(clientId, info);
    }

    info->workerTh->start();

    bool ret{false};
    QMetaObject::invokeMethod(
        info->worker.get(), [clientSocket, &ret, worker = info->worker.get()]() { ret = worker->InitSocket(clientSocket); },
        Qt::BlockingQueuedConnection);

    if (!ret) {
        // 清理资源。
    }

    // 发送客户端ID
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->type = FBT_SER_MSG_YOURID;
    frame->fid = id_;
    frame->tid = clientId;
    frame->data = clientId.toUtf8();
    dataDispatcher(clientId, frame);

    qInfo() << "Client connected:" << clientId << "Total clients:" << clients_.size();
}

QByteArray Server::getClients()
{
    InfoClientVec infoClients;

    {
        QReadLocker locker(&rwLock_);
        for (auto& c : clients_) {
            InfoClient infoClient;
            infoClient.id = c->worker->GetID();
            infoClients.vec.append(infoClient);
        }
    }

    auto ret = infoPack<InfoClientVec>(infoClients);
    return ret;
}

bool Server::dataDispatcher(const QString& id, QSharedPointer<FrameBuffer> frame)
{
    QSharedPointer<ClientInfo> cli;

    {
        QReadLocker locker(&rwLock_);
        if (clients_.contains(id)) {
            cli = clients_[id];
        }
    }

    if (!cli) {
        return false;
    }

    return QMetaObject::invokeMethod(cli->worker.get(), "onDataReadyOutIn", Qt::BlockingQueuedConnection,
                                     Q_ARG(QSharedPointer<FrameBuffer>, frame));
}
