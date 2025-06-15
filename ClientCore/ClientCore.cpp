#include "ClientCore.h"

#include <QDebug>

ClientCore::ClientCore(QObject* parent) : QObject(parent)
{
}

ClientCore::~ClientCore()
{
}

bool ClientCore::Connect(const QString& ip, quint16 port)
{
    QMutexLocker locker(&conMutex_);
    if (!locker.isLocked()) {
        qWarning() << QString(tr("%1:%2 already connecting...")).arg(ip).arg(port);
        return false;
    }
    socket_->connectToHost(ip, port);
    if (!socket_->waitForConnected(3000)) {
        qCritical() << QString(tr("%1:%2 connect failed...")).arg(ip).arg(port);
        return false;
    }
    return true;
}

void ClientCore::Disconnect()
{
}

void ClientCore::onReadyRead()
{
    QByteArray data = socket_->readAll();
    recvBuffer_.append(data);
    while (true) {
        auto frame = Protocol::ParseBuffer(recvBuffer_);
        if (frame == nullptr) {
            break;
        }
        UseFrame(frame);
    }
}

void ClientCore::onDisconnected()
{
    qCritical() << QString("client %1 disconnected...").arg(remoteID_);
}

void ClientCore::UseFrame(QSharedPointer<FrameBuffer> frame)
{
}

bool ClientCore::Send(QSharedPointer<FrameBuffer> frame)
{
    if (frame == nullptr) {
        return false;
    }
    auto data = Protocol::PackBuffer(frame);
    if (data.size() == 0) {
        return false;
    }
    return Send(data.constData(), data.size());
}

bool ClientCore::Send(const char* data, qint64 len)
{
    if (socket_->state() != QAbstractSocket::ConnectedState) {
        qCritical() << QString("client %1 not connected...").arg(remoteID_);
        return false;
    }
    qint64 bytesWritten = socket_->write(data, len);
    if (bytesWritten == -1 || !socket_->waitForBytesWritten(5000)) {
        qCritical() << QString("Send data to server failed. %1").arg(socket_->errorString());
        return false;
    }
    return true;
}

void ClientCore::SetClientsCall(const std::function<void(const InfoClientVec& clients)>& call)
{
}

void ClientCore::SetPathCall(const std::function<void(const QString& path)>& call)
{
}

void ClientCore::SetFileCall(const std::function<void(const DirFileInfoVec& files)>& call)
{
}

void ClientCore::SetRemoteID(const QString& id)
{
}

QString ClientCore::GetRemoteID()
{
    return remoteID_;
}
