#include "ClientCore.h"

#include <QDebug>

ClientCore::ClientCore(QObject* parent) : QObject(parent)
{
    socket_ = new QTcpSocket(this);
    connect(socket_, &QTcpSocket::readyRead, this, &ClientCore::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &ClientCore::onDisconnected);
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
    qInfo() << QString(tr("%1:%2 connected success.")).arg(ip).arg(port);
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
    emit sigDisconnect();
}

void ClientCore::UseFrame(QSharedPointer<FrameBuffer> frame)
{
    switch (frame->type) {
    case FrameBufferType::FBT_SER_MSG_ASKCLIENTS: {
        InfoClientVec info = infoUnpack<InfoClientVec>(frame->data);
        emit sigClients(info);
        break;
    }
    case FrameBufferType::FBT_SER_MSG_YOURID: {
        ownID_ = frame->data;
        qInfo() << QString(tr("own id: %1")).arg(ownID_);
        break;
    }
    case FrameBufferType::FBT_CLI_ANS_DIRFILE: {
        DirFileInfoVec info = infoUnpack<DirFileInfoVec>(frame->data);
        emit sigFiles(info);
        break;
    }
    case FrameBufferType::FBT_CLI_ASK_DIRFILE: {
        DirFileInfoVec vec;
        InfoMsg info = infoUnpack<InfoMsg>(frame->data);
        if (!localFile_.GetDirFile(info.msg, vec)) {
            qWarning() << QString(tr("get dir file failed use %1")).arg(info.msg);
            return;
        }
        if (!Send<DirFileInfoVec>(vec, FBT_CLI_ANS_DIRFILE, frame->fid)) {
            qCritical() << QString(tr("send dir file result failed."));
            return;
        }
        break;
    }
    case FrameBufferType::FBT_CLI_ASK_HOME: {
        InfoMsg info;
        info.msg = Util::GetUserHome();
        auto data = infoPack<InfoMsg>(info);
        if (!Send<InfoMsg>(info, FBT_CLI_ANS_HOME, frame->fid)) {
            qCritical() << QString(tr("send home failed."));
            return;
        }
        break;
    }
    case FrameBufferType::FBT_CLI_ANS_HOME: {
        InfoMsg info = infoUnpack<InfoMsg>(frame->data);
        qInfo() << QString(tr("home: %1")).arg(info.msg);
        emit sigPath(info.msg);
        break;
    }
    case FrameBufferType::FBT_SER_MSG_FORWARD_FAILED: {
        qCritical() << QString(tr("************************** forward failed."));
        break;
    }
    case FrameBufferType::FBT_CLI_REQ_SEND: {
        emit sigReqSend(frame);
        break;
    }
    case FrameBufferType::FBT_CLI_REQ_DOWN: {
        emit sigReqDown(frame);
        break;
    }
    case FrameBufferType::FBT_CLI_TRANS_DONE: {
        emit sigTransDone(frame);
        break;
    }
    case FrameBufferType::FBT_CLI_CAN_SEND: {
        emit sigCanSend(frame);
        break;
    }
    case FrameBufferType::FBT_CLI_CANOT_SEND: {
        emit sigCanotSend(frame);
        break;
    }
    case FBT_CLI_CANOT_DOWN: {
        emit sigCanotDown(frame);
        break;
    }
    case FBT_CLI_CAN_DOWN: {
        emit sigCanDown(frame);
        break;
    }
    case FBT_CLI_FILE_BUFFER: {
        emit sigFileBuffer(frame);
        break;
    }
    case FBT_CLI_TRANS_FAILED: {
        emit sigTransFailed(frame);
        break;
    }
    default:
        qCritical() << QString("unknown frame type: %1").arg(frame->type);
        break;
    }
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

    qint64 bytesWritten = -1;
    {
        QMutexLocker locker(&sockMut_);
        bytesWritten = socket_->write(data, len);
    }
    if (bytesWritten == -1 || !socket_->waitForBytesWritten(5000)) {
        qCritical() << QString("Send data to server failed. %1").arg(socket_->errorString());
        return false;
    }
    return true;
}

void ClientCore::SetRemoteID(const QString& id)
{
    remoteID_ = id;
}

QString ClientCore::GetRemoteID()
{
    return remoteID_;
}

QString ClientCore::GetOwnID()
{
    return ownID_;
}

SocketWorker::SocketWorker(ClientCore* core, QObject* parent) : QThread(parent), core_(core)
{
    connect(core_, &ClientCore::sigDisconnect, this, [this]() {
        emit disconnected();
        thread()->quit();
    });
}

SocketWorker::~SocketWorker()
{
}

void SocketWorker::SetConnectInfo(const QString& ip, qint16 port)
{
    ip_ = ip;
    port_ = port;
}

void SocketWorker::run()
{
    emit connecting();
    if (!core_->Connect(ip_, port_)) {
        emit conFailed();
        return;
    }
    emit conSuccess();
    exec();
}
