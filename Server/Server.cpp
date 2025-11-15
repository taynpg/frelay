#include "Server.h"

#include <QDateTime>
#include <QDebug>

#include "InfoClient.h"
#include "InfoMsg.h"
#include "InfoPack.hpp"

#define NO_HEATBEAT_TIMEOUT (10)
#define MONITOR_HEART_SPED (10 * 2)

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
        client->socket->disconnectFromHost();
        client->socket->deleteLater();
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
        return;
    }

    auto client = QSharedPointer<ClientInfo>::create();
    client->socket = clientSocket;
    client->socket->setProperty("clientId", clientId);
    client->id = clientId;
    // client->connectTime = QDateTime::currentSecsSinceEpoch();
    client->connectTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000;

    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onClientDisconnected);

    {
        QWriteLocker locker(&rwLock_);
        clients_.insert(clientId, client);
        flowBackCount_[clientId] = 0;
    }

    qInfo() << "Client connected:" << clientId;
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->type = FBT_SER_MSG_YOURID;
    frame->fid = id_;
    frame->tid = clientId;
    frame->data = clientId.toUtf8();
    sendData(clientSocket, frame);
}

void Server::onReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    QSharedPointer<ClientInfo> client;

    {
        QReadLocker locker(&rwLock_);
        client = clients_.value(socket->property("clientId").toString());
    }

    if (client) {
        if (client->buffer.size() > MAX_INVALID_PACKET_SIZE) {
            auto mg = QString("Client %1 buffer size exceeded, XXXXX...").arg(client->id);
            qWarning() << mg;
            socket->disconnectFromHost();
            return;
        }
        client->buffer.append(socket->readAll());
        processClientData(client);
    }
}

void Server::processClientData(QSharedPointer<ClientInfo> client)
{
    while (true) {
        auto frame = Protocol::ParseBuffer(client->buffer);
        if (frame.isNull()) {
            break;
        }
        frame->fid = client->id;
        if (frame->type <= 30) {
            // frame->tid = "server";
            replyRequest(client, frame);
        } else {
            if (!forwardData(client, frame)) {
                qWarning() << "Failed to forward data from" << client->id << "to" << frame->tid;
            }
        }
    }
}

bool Server::sendWithFlowCheck(QTcpSocket* fsoc, QTcpSocket* tsoc, QSharedPointer<FrameBuffer> frame)
{
    auto flowLimit = [this](QTcpSocket* fsoc, BlockLevel aLevel) {
        InfoMsg msg;
        msg.mark = static_cast<qint32>(aLevel);
        auto f = QSharedPointer<FrameBuffer>::create();
        f->type = FBT_SER_FLOW_LIMIT;
        f->data = infoPack<InfoMsg>(msg);
        if (!sendData(fsoc, f)) {
            qWarning() << "Failed to send flow limit message to" << fsoc->property("clientId").toString();
        }
    };

    if (flowBackCount_[fsoc->property("clientId").toString()] > FLOW_BACK_MULTIPLE) {
        auto level = getBlockLevel(tsoc);
        flowLimit(fsoc, level);
        // qDebug() << "Flow back count exceeded, block level:" << level;
    }
    flowBackCount_[fsoc->property("clientId").toString()]++;
    return sendData(tsoc, frame);
}

bool Server::forwardData(QSharedPointer<ClientInfo> client, QSharedPointer<FrameBuffer> frame)
{
    QSharedPointer<ClientInfo> targetClient;
    QSharedPointer<ClientInfo> fromClient;

    {
        QReadLocker locker(&rwLock_);
        targetClient = clients_.value(frame->tid);
        fromClient = clients_.value(frame->fid);
    }

    if (targetClient && fromClient) {
        return sendWithFlowCheck(fromClient->socket, targetClient->socket, frame);
    } else {
        auto errorFrame = QSharedPointer<FrameBuffer>::create();
        errorFrame->type = FBT_SER_MSG_FORWARD_FAILED;
        errorFrame->fid = id_;
        errorFrame->tid = client->id;
        errorFrame->data = QString("Target client %1 not found").arg(frame->tid).toUtf8();
        return sendData(client->socket, errorFrame);
    }
}

void Server::replyRequest(QSharedPointer<ClientInfo> client, QSharedPointer<FrameBuffer> frame)
{
    switch (frame->type) {
    case FBT_SER_MSG_ASKCLIENTS: {
        QByteArray clientList = getClients();
        auto replyFrame = QSharedPointer<FrameBuffer>::create();
        replyFrame->type = FBT_SER_MSG_ASKCLIENTS;
        replyFrame->fid = id_;
        replyFrame->tid = client->id;
        replyFrame->data = clientList;
        auto ret = sendData(client->socket, replyFrame);
        break;
    }
    case FBT_SER_MSG_HEARTBEAT: {
        QSharedPointer<ClientInfo> cl;
        {
            QReadLocker locker(&rwLock_);
            if (clients_.count(frame->fid)) {
                cl = clients_.value(frame->fid);
            }
        }
        if (cl) {
            // qDebug() << "Client" << cl->id << "heartbeat received";
            cl->connectTime = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000;
        }
        break;
    }
    case FBT_SER_MSG_JUDGE_OTHER_ALIVE: {
        QSharedPointer<ClientInfo> cl;
        {
            QReadLocker locker(&rwLock_);
            if (clients_.count(frame->tid)) {
                cl = clients_.value(frame->tid);
            }
        }
        if (!cl) {
            auto rf = QSharedPointer<FrameBuffer>::create();
            rf->type = FBT_SER_MSG_OFFLINE;
            rf->fid = frame->tid;
            rf->tid = frame->fid;
            sendData(client->socket, rf);
        }
        break;
    }
    default:
        qWarning() << "Unknown request type:" << frame->type;
        break;
    }
}

bool Server::sendData(QTcpSocket* socket, QSharedPointer<FrameBuffer> frame)
{
    if (!socket || !socket->isOpen() || frame.isNull()) {
        return false;
    }

    QByteArray data = Protocol::PackBuffer(frame);
    if (data.isEmpty()) {
        return false;
    }
    return socket->write(data) == data.size();
}

BlockLevel Server::getBlockLevel(QTcpSocket* socket)
{
    auto b = socket->bytesToWrite();
    constexpr auto one = CHUNK_BUF_SIZE;
    if (b > one * 1000) {
        return BL_LEVEL_8;
    }
    if (b > one * 200) {
        return BL_LEVEL_7;
    }
    if (b > one * 100) {
        return BL_LEVEL_6;
    }
    if (b > one * 50) {
        return BL_LEVEL_5;
    }
    if (b > one * 30) {
        return BL_LEVEL_4;
    }
    if (b > one * 10) {
        return BL_LEVEL_3;
    }
    if (b > one * 5) {
        return BL_LEVEL_2;
    }
    if (b > one * 1) {
        return BL_LEVEL_1;
    }
    return BL_LEVEL_NORMAL;
}

void Server::onClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    QString clientId = socket->property("clientId").toString();

    {
        QWriteLocker locker(&rwLock_);
        if (clients_.count(clientId)) {
            clients_.remove(clientId);
        }
        if (flowBackCount_.count(clientId)) {
            flowBackCount_.remove(clientId);
        }
    }

    qInfo() << "Client disconnected:" << __LINE__ << clientId;
    socket->deleteLater();
}

void Server::monitorClients()
{
    qint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000;
    std::vector<QTcpSocket*> prepareRemove;

    {
        QReadLocker locker(&rwLock_);
        for (auto& c : clients_) {
            if ((now - c->connectTime) > NO_HEATBEAT_TIMEOUT) {
                prepareRemove.push_back(c->socket);
            }
        }
    }
    for (const auto& s : prepareRemove) {
        qInfo() << "Removing inactive client:" << s->property("clientId").toString();
        s->disconnectFromHost();
    }
}

QByteArray Server::getClients()
{
    InfoClientVec infoClients;

    {
        QReadLocker locker(&rwLock_);
        for (auto& c : clients_) {
            InfoClient infoClient;
            infoClient.id = c->id;
            infoClients.vec.append(infoClient);
        }
    }

    auto ret = infoPack<InfoClientVec>(infoClients);
    return ret;
}
