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
    clearWaitTimer_ = new QTimer(this->parent());
    clearWaitTimer_->setInterval(1000);
    connect(clearWaitTimer_, &QTimer::timeout, this, [this]() { clearWaitTask(); });
    clearWaitTimer_->start();
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
                } else {
                    item.state = static_cast<qint32>(FCS_FILE_EXIST);
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
    if (msg.command == STRMSG_AC_RENAME_FILEDIR) {
        msg.command = STRMSG_AC_ANSWER_RENAME_FILEDIR;
        msg.msg = Util::Rename(msg.fromPath, msg.toPath, msg.type == STR_DIR);
        if (!Send<InfoMsg>(msg, FBT_MSGINFO_ANSWER, frame->fid)) {
            auto logMsg = tr("给") + frame->fid + tr("返回重命名结果消息失败。");
            qCritical() << logMsg;
            return;
        }
        return;
    }
    if (msg.command == STRMSG_AC_DEL_FILEDIR) {
        msg.command = STRMSG_AC_ANSWER_DEL_FILEDIR;
        auto& delItemsMap = msg.infos;
        msg.msg.clear();
        for (auto& key : delItemsMap.keys()) {
            for (auto& delItem : delItemsMap[key]) {
                qWarning() << frame->fid << "正在删除：" << delItem.path;
                msg.msg = Util::Delete(delItem.path);
                if (!msg.msg.isEmpty()) {
                    break;
                } else {
                    // 标记1表示删除成功。
                    delItem.state = 1;
                }
            }
            if (!msg.msg.isEmpty()) {
                break;
            }
        }
        if (!Send<InfoMsg>(msg, FBT_MSGINFO_ANSWER, frame->fid)) {
            auto logMsg = tr("给") + frame->fid + tr("返回删除结果消息失败。");
            qCritical() << logMsg;
            return;
        }
        return;
    }
    if (msg.command == STRMSG_AC_NEW_DIR) {
        msg.command = STRMSG_AC_ANSWER_NEW_DIR;
        msg.msg = Util::NewDir(msg.fromPath);
        if (!Send<InfoMsg>(msg, FBT_MSGINFO_ANSWER, frame->fid)) {
            auto logMsg = tr("给") + frame->fid + tr("返回新建结果消息失败。");
            qCritical() << logMsg;
            return;
        }
        return;
    }
    // 这个请求的处理可能是耗时的，需要开线程处理。
    if (msg.command == STRMSG_AC_ALL_DIRFILES) {
        msg.command = STRMSG_AC_ANSWER_ALL_DIRFILES;
        auto taskKey = frame->fid + "=" + STRMSG_AC_ALL_DIRFILES;
        QMutexLocker locker(&waitTaskMut_);
        if (waitTask_.contains(taskKey)) {
            msg.msg = STRMSG_ST_COMMAND_ALREADY_RUNNING;
            if (!Send<InfoMsg>(msg, FBT_MSGINFO_ANSWER, frame->fid)) {
                auto logMsg = tr("给") + frame->fid + tr("返回获取文件列表结果消息失败。");
                qCritical() << logMsg;
                return;
            }
        } else {
            waitTask_[taskKey] = WaitTask();
            auto& wt = waitTask_[taskKey];
            QString fid = frame->fid;
            wt.wo = new WaitOperOwn(this);
            wt.wo->SetClient(this);
            wt.wo->fid = fid;
            wt.wo->infoMsg_ = msg;
            wt.wo->func_ = [this, &wt, fid]() {
                auto& infoMsg = wt.wo->infoMsg_;
                infoMsg.command = STRMSG_AC_ANSWER_ALL_DIRFILES;
                bool success = false;
                // infoMsg.infos.clear();
                for (auto& item : infoMsg.infos.keys()) {
                    auto fullDir = Util::Join(infoMsg.fst.root, item);
                    if (!DirFileHelper::GetAllFiles(fullDir, infoMsg.list)) {
                        success = false;
                        break;
                    }
                    auto& vec = infoMsg.infos[item];
                    for (const auto& dd : std::as_const(infoMsg.list)) {
                        FileStruct fst;
                        fst.root = infoMsg.fst.root;
                        fst.mid = item;
                        fst.relative = dd;
                        vec.push_back(fst);
                    }
                }
                return success;
            };
            wt.wo->start();
        }
        return;
    }
    if (msg.command == STRMSG_AC_ASK_SHA256) {
        msg.command = STRMSG_AC_ANSWER_SHA256;
        auto taskKey = frame->fid + "=" + STRMSG_AC_ASK_SHA256;
        QMutexLocker locker(&waitTaskMut_);
        if (waitTask_.contains(taskKey)) {
            msg.msg = STRMSG_ST_COMMAND_ALREADY_RUNNING;
            if (!Send<InfoMsg>(msg, FBT_MSGINFO_ANSWER, frame->fid)) {
                auto logMsg = tr("给") + frame->fid + tr("返回获取SHA256结果消息失败。");
                qCritical() << logMsg;
                return;
            }
        } else {
            waitTask_[taskKey] = WaitTask();
            auto& wt = waitTask_[taskKey];
            QString fid = frame->fid;
            wt.wo = new WaitOperOwn(this);
            wt.wo->SetClient(this);
            wt.wo->fid = fid;
            wt.wo->infoMsg_ = msg;
            wt.wo->func_ = [this, &wt, fid]() {
                auto& infoMsg = wt.wo->infoMsg_;
                infoMsg.command = STRMSG_AC_ANSWER_SHA256;
                bool success = false;
                auto sha256 = Util::GenSha256(infoMsg.fst.path);
                if (sha256.isEmpty()) {
                    infoMsg.msg = QString("获取%1的sha256失败。").arg(infoMsg.fst.path);
                } else {
                    infoMsg.msg.clear();
                    infoMsg.fst.mark = sha256;
                }
                return success;
            };
            wt.wo->start();
        }
        return;
    }

    // 未知信息
    qWarning() << QString(tr("未知询问信息类型：%1")).arg(msg.command);
}

void ClientCore::clearWaitTask()
{
    QMutexLocker locker(&waitTaskMut_);
    QList<QString> completedTasks;

    for (auto it = waitTask_.begin(); it != waitTask_.end(); ++it) {
        WaitTask& task = it.value();
        if (task.wo && task.wo->isFinished()) {
            completedTasks.append(it.key());
        }
    }

    for (const QString& taskId : completedTasks) {
        auto it = waitTask_.find(taskId);
        if (it != waitTask_.end()) {
            WaitTask& task = it.value();
            if (task.wo) {
                task.wo->wait();
                delete task.wo;
                task.wo = nullptr;
            }
            waitTask_.erase(it);
            qDebug() << "清理完成的任务:" << taskId;
        }
    }
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
        // qDebug() << "来自" << frame->fid << "的文件夹请求。。。";
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
        // qDebug() << "来自" << frame->fid << "的HOME请求。。。";
        InfoMsg info;
        info.msg = Util::GetUserHome();
        info.list = Util::GetLocalDrivers();
        if (!Send<InfoMsg>(info, FBT_CLI_ANS_HOME, frame->fid)) {
            qCritical() << QString(tr("send home failed."));
            return;
        }
        break;
    }
    case FrameBufferType::FBT_CLI_ANS_HOME: {
        InfoMsg info = infoUnpack<InfoMsg>(frame->data);
        qInfo() << QString(tr("用户目录：%1")).arg(info.msg);
        emit sigPath(info.msg, info.list);
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
        popID(frame->fid);
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
    case FBT_CLI_TRANS_INTERRUPT: {
        emit sigTransInterrupt(frame);
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
    // 这里 1000 * 300 应对网络非常慢的情况。
    if (bytesWritten == -1 || !socket_->waitForBytesWritten(1000 * 300)) {
        qCritical() << QString("向服务器发送数据失败:[%1], written:[%2]").arg(socket_->errorString()).arg(bytesWritten);
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

void ClientCore::pushID(const QString& id)
{
    QWriteLocker locker(&rwIDLock_);
    if (!remoteIDs_.contains(id)) {
        remoteIDs_.push_back(id);
    }
}

void ClientCore::popID(const QString& id)
{
    QWriteLocker locker(&rwIDLock_);
    remoteIDs_.removeAll(id);
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
        if (!rid.isEmpty()) {
            auto frame2 = core_->GetBuffer(info, FBT_SER_MSG_JUDGE_OTHER_ALIVE, rid);
            ClientCore::syncInvoke(core_, frame2);
        }

        {
            QReadLocker loker(&core_->rwIDLock_);
            for (auto& id : core_->remoteIDs_) {
                auto frame3 = core_->GetBuffer(info, FBT_SER_MSG_JUDGE_OTHER_ALIVE, id);
                ClientCore::syncInvoke(core_, frame3);
            }
        }
    }
}

WaitThread::WaitThread(QObject* parent) : QThread(parent)
{
}

void WaitThread::SetClient(ClientCore* cli)
{
    cli_ = cli;
}

bool WaitThread::IsQuit() const
{
    return isAlreadyInter_;
}

void WaitThread::interrupCheck()
{
    isRun_ = false;
    if (!isAlreadyInter_) {
        isAlreadyInter_ = true;
        emit sigCheckOver();
    }
}

WaitOper::WaitOper(QObject* parent) : WaitThread(parent)
{
}

void WaitOper::run()
{
    isAlreadyInter_ = false;
    infoMsg_.msg = STR_NONE;
    isRun_ = true;
    recvMsg_ = false;

    infoMsg_.command = sendStrType_;
    infoMsg_.fromPath = stra_;
    infoMsg_.toPath = strb_;
    infoMsg_.type = type_;

    auto f = cli_->GetBuffer<InfoMsg>(infoMsg_, FBT_MSGINFO_ASK, cli_->GetRemoteID());
    if (!ClientCore::syncInvoke(cli_, f)) {
        auto errMsg = QString(tr("向%1发送%2请求失败。")).arg(cli_->GetRemoteID()).arg(sendStrType_);
        emit sigCheckOver();
        qCritical() << errMsg;
        return;
    }
    while (isRun_) {
        QThread::msleep(1);
        if (isAlreadyInter_) {
            qInfo() << tr("线程中断文件操作等待......");
            return;
        }
        if (!recvMsg_) {
            continue;
        }
        break;
    }
    isAlreadyInter_ = true;
    emit sigCheckOver();
    auto n = QString(tr("向%1的请求%2处理结束。")).arg(cli_->GetRemoteID()).arg(sendStrType_);
    qInfo() << n;
}

void WaitOper::SetType(const QString& sendType, const QString& ansType)
{
    sendStrType_ = sendType;
    ansStrType_ = ansType;
}

void WaitOper::SetPath(const QString& stra, const QString& strb, const QString& type)
{
    stra_ = stra;
    strb_ = strb;
    type_ = type;
}

InfoMsg WaitOper::GetMsgConst() const
{
    return infoMsg_;
}

InfoMsg& WaitOper::GetMsgRef()
{
    return infoMsg_;
}

void WaitOper::interrupCheck()
{
    // qWarning() << QString(tr("中断请求处理%1......")).arg(sendStrType_);
    WaitThread::interrupCheck();
}

void WaitOper::recvFrame(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    if (info.command == ansStrType_) {
        infoMsg_ = info;
        recvMsg_ = true;
        return;
    }
    auto n = tr("收到未知Oper的回复信息：") + info.command;
    qInfo() << n;
}

WaitOperOwn::WaitOperOwn(QObject* parent) : WaitThread(parent)
{
}

void WaitOperOwn::run()
{
    auto execRet = false;
    if (func_) {
        execRet = func_();
    }
    if (!fid.isEmpty()) {
        if (!cli_->syncInvoke(cli_, cli_->GetBuffer<InfoMsg>(infoMsg_, FBT_MSGINFO_ANSWER, fid))) {
            qCritical() << QString(tr("向%1发送%2请求失败。")).arg(fid).arg(infoMsg_.command);
        }
    }
    emit sigOver();
}

void WaitOperOwn::recvFrame(QSharedPointer<FrameBuffer> frame)
{
    qDebug() << "不应该被调用的地方：" << __FUNCTION__;
}
