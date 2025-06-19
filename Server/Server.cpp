#include "Server.h"

#include <QDateTime>
#include <QDebug>

#include "InfoClient.h"
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

    qDebug() << "Server started on port" << serverPort();
    monitorTimer_->start(30000);
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
    client->connectTime = QDateTime::currentSecsSinceEpoch();

    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onClientDisconnected);

    {
        QWriteLocker locker(&rwLock_);
        clients_.insert(clientId, client);
    }

    qDebug() << "Client connected:" << clientId;
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->type = FBT_SER_MSG_YOURID;
    frame->fid = id_;
    frame->tid = clientId;
    frame->data = clientId.toUtf8();
    sendData(clientSocket, frame);
}

void Server::onClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    QHostAddress peerAddress = socket->peerAddress();
    quint32 ipv4 = peerAddress.toIPv4Address();
    QString ipStr = QHostAddress(ipv4).toString();
    QString clientId = QString("%1:%2").arg(ipStr).arg(socket->peerPort());

    {
        QWriteLocker locker(&rwLock_);
        clients_.remove(clientId);
    }

    qDebug() << "Client disconnected:" << clientId;
    socket->deleteLater();
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
            frame->tid = "server";
            replyRequest(client, frame);
        } else {
            if (!forwardData(client, frame)) {
                qWarning() << "Failed to forward data from" << client->id << "to" << frame->tid;
            }
        }
    }
}

bool Server::forwardData(QSharedPointer<ClientInfo> client, QSharedPointer<FrameBuffer> frame)
{
    QSharedPointer<ClientInfo> targetClient;

    {
        QReadLocker locker(&rwLock_);
        targetClient = clients_.value(frame->tid);
    }

    if (targetClient) {
        return sendData(targetClient->socket, frame);
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
        qDebug() << "Reply client list:" << client->id << ret;
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

void Server::monitorClients()
{
    qint64 now = QDateTime::currentSecsSinceEpoch();
    QWriteLocker locker(&rwLock_);

    for (auto it = clients_.begin(); it != clients_.end();) {
        if (now - it.value()->connectTime > 300) {
            qDebug() << "Disconnecting inactive client:" << it.value()->id;
            it.value()->socket->disconnectFromHost();
            it = clients_.erase(it);
        } else {
            ++it;
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
            infoClient.id = c->id;
            infoClients.vec.append(infoClient);
        }
    }

    auto ret = infoPack<InfoClientVec>(infoClients);
    return ret;
}
