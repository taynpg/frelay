#include "ClientCore.h"

#include <QDebug>

ClientCore::ClientCore(QObject* parent) : QObject(parent)
{
}

void ClientCore::Instance()
{
    // qDebug() << "Instance() thread:" << QThread::currentThread();
    socket_ = new QTcpSocket(this);
    connect(socket_, &QTcpSocket::readyRead, this, &ClientCore::onReadyRead);
    connect(socket_, &QTcpSocket::disconnected, this, &ClientCore::onDisconnected);
}

ClientCore::~ClientCore()
{
}

bool ClientCore::SendFrame(QSharedPointer<FrameBuffer> frame)
{
    return Send(frame);
}

void ClientCore::DoConnect(const QString& ip, quint16 port)
{
    // qDebug() << "doConnect thread:" << QThread::currentThread();
    emit connecting();
    if (!Connect(ip, port)) {
        emit conFailed();
        return;
    }
    emit conSuccess();
}

bool ClientCore::Connect(const QString& ip, quint16 port)
{
    if (connected_) {
        qInfo() << QString(tr("已连接。"));
        return true;
    }
    socket_->connectToHost(ip, port);
    if (!socket_->waitForConnected(3000)) {
        qCritical() << QString(tr("%1:%2 连接失败。")).arg(ip).arg(port);
        return false;
    }
    qInfo() << QString(tr("%1:%2 连接成功。")).arg(ip).arg(port);
    connected_ = true;
    return true;
}

void ClientCore::Disconnect()
{
    QMutexLocker locker(&conMutex_);
    if (socket_ && socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->disconnectFromHost();
        if (socket_->state() != QAbstractSocket::UnconnectedState) {
            socket_->waitForDisconnected(1000);
        }
    }
    connected_ = false;
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
    connected_ = false;
    qCritical() << QString("你 [%1] 断开了。").arg(ownID_);
    emit sigDisconnect();
}

void ClientCore::handleAsk(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg msg = infoUnpack<InfoMsg>(frame->data);
    // TODO: 处理询问请求
    if (msg.command == STRMSG_AC_CHECK_FILE_EXIST) {
        msg.command = STRMSG_AC_ANSWER_FILE_EXIST;
        for (auto& item : msg.mapData) {
            if (item.command == STRMSG_AC_UP) {
                if (!Util::DirExist(item.remotePath, false)) {
                    item.state = static_cast<qint32>(FCS_DIR_NOT_EXIST);
                    continue;
                }
                auto newerPath = Util::Get2FilePath(item.localPath, item.remotePath);
                if (Util::FileExist(newerPath)) {
                    item.state = static_cast<qint32>(FCS_FILE_EXIST);
                    continue;
                }
            } else {
                if (!Util::FileExist(item.remotePath)) {
                    item.state = static_cast<qint32>(FCS_FILE_NOT_EXIST);
                }
            }
        }
        if (!Send<InfoMsg>(msg, FBT_MSGINFO_ANSWER, frame->fid)) {
            auto logMsg = tr("给") + frame->fid + tr("返回检查文件存在性消息失败。");
            qCritical() << logMsg;
            return;
        }
        return;
    }
    // 未知信息
    qWarning() << QString(tr("未知询问信息类型：%1")).arg(msg.command);
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
        GlobalData::Ins()->SetLocalID(ownID_);
        qInfo() << QString(tr("本机ID: %1")).arg(ownID_);
        emit sigYourId(frame);
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
            qWarning() << QString(tr("访问文件失败： %1")).arg(info.msg);
            return;
        }
        if (!Send<DirFileInfoVec>(vec, FBT_CLI_ANS_DIRFILE, frame->fid)) {
            qCritical() << QString(tr("发送文件列表结果失败。"));
            return;
        }
        break;
    }
    case FrameBufferType::FBT_CLI_ASK_HOME: {
        InfoMsg info;
        info.msg = Util::GetUserHome();
        if (!Send<InfoMsg>(info, FBT_CLI_ANS_HOME, frame->fid)) {
            qCritical() << QString(tr("send home failed."));
            return;
        }
        break;
    }
    case FrameBufferType::FBT_CLI_ANS_HOME: {
        InfoMsg info = infoUnpack<InfoMsg>(frame->data);
        qInfo() << QString(tr("用户目录：%1")).arg(info.msg);
        emit sigPath(info.msg);
        break;
    }
    case FrameBufferType::FBT_SER_MSG_FORWARD_FAILED: {
        qCritical() << QString(tr("转发数据失败，fid:%1, tid:%2, type:%3"))
                           .arg(frame->fid)
                           .arg(frame->tid)
                           .arg(static_cast<uint32_t>(frame->type));
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
    case FBT_CLI_FILE_INFO: {
        emit sigFileInfo(frame);
        break;
    }
    case FBT_SER_MSG_OFFLINE: {
        emit sigOffline(frame);
        break;
    }
    case FBT_MSGINFO_ASK: {
        handleAsk(frame);
        break;
    }
    case FBT_MSGINFO_ANSWER: {
        emit sigMsgAnswer(frame);
        break;
    }
    case FBT_SER_FLOW_LIMIT: {
        emit sigFlowBack(frame);
        break;
    }
    default:
        qCritical() << QString("未知的帧类型： %1").arg(frame->type);
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
        qCritical() << QString("客户端 %1 未连接...").arg(remoteID_);
        return false;
    }

    qint64 bytesWritten = -1;
    {
        QMutexLocker locker(&sockMut_);
        bytesWritten = socket_->write(data, len);
    }
    if (bytesWritten == -1 || !socket_->waitForBytesWritten(5000)) {
        qCritical() << QString("向服务器发送数据失败： %1").arg(socket_->errorString());
        return false;
    }
    return true;
}

bool ClientCore::IsConnect()
{
    return connected_;
}

void ClientCore::SetRemoteID(const QString& id)
{
    GlobalData::Ins()->SetRemoteID(id);
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
    // connect(core_, &ClientCore::sigDisconnect, this, [this]() {
    //     thread()->quit();
    // });
}

SocketWorker::~SocketWorker()
{
}

void SocketWorker::run()
{
    core_->Instance();
    exec();
}

HeatBeat::HeatBeat(ClientCore* core, QObject* parent) : QThread(parent), core_(core)
{
}

HeatBeat::~HeatBeat()
{
    Stop();
}

void HeatBeat::Stop()
{
    isRun_ = false;
}

void HeatBeat::run()
{
    isRun_ = true;
    InfoMsg info;
    auto frame = core_->GetBuffer(info, FBT_SER_MSG_HEARTBEAT, "");
    while (isRun_) {
        QThread::sleep(1);
        if (!core_->IsConnect()) {
            continue;
        }
        ClientCore::syncInvoke(core_, frame);
        auto rid = core_->GetRemoteID();
        if (rid.isEmpty()) {
            continue;
        }
        auto frame2 = core_->GetBuffer(info, FBT_SER_MSG_JUDGE_OTHER_ALIVE, rid);
        ClientCore::syncInvoke(core_, frame2);
    }
}