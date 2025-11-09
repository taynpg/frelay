#include "FileTrans.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

FileTrans::FileTrans(ClientCore* clientCore) : clientCore_(clientCore)
{
    RegisterSignal();
    downTask_ = QSharedPointer<DoTransTask>::create();
    sendTask_ = QSharedPointer<DoTransTask>::create();
}

/*
 *   When the local client actively sends a file, it cannot simultaneously perform download operations.
 *   Passive sending is an exception, primarily to monitor the sending progress. In contrast,
 *   the progress of passive sending is monitored by the requesting download side,
 *   so there is no need to track the progress locally.
 */
void FileTrans::ReqSendFile(const TransTask& task)
{
    sendTask_->state = TaskState::STATE_RUNNING;
    // TODO: check if running...
    if (sendTask_->file.isOpen()) {
        qWarning() << QString(tr("文件 [%1] 已经被打开，将会自动关闭。")).arg(sendTask_->file.fileName());
        sendTask_->file.close();
    }
    // start
    InfoMsg info;
    info.toPath = task.remotePath;
    info.fromPath = task.localPath;

    sendTask_->file.setFileName(info.fromPath);
    if (!sendTask_->file.open(QIODevice::ReadOnly)) {
        qCritical() << QString(tr("打开 [%1] 失败。")).arg(info.fromPath);
        sendTask_->state = TaskState::STATE_NONE;
        return;
    }

    QFileInfo fileInfo(info.fromPath);
    if (fileInfo.exists()) {
        qint64 size = fileInfo.size();
        if (size == 0) {
            qCritical() << QString(tr("文件 [%1] 尺寸为0不会进行发送。")).arg(info.fromPath);
            sendTask_->file.close();
            sendTask_->state = TaskState::STATE_FINISH;
            return;
        }
        info.permissions = static_cast<quint32>(fileInfo.permissions());
        info.size = size;
    } else {
        qCritical() << QString(tr("文件 [%1] 不存在。")).arg(info.fromPath);
        sendTask_->file.close();
        return;
    }

    auto frame = clientCore_->GetBuffer(info, FBT_CLI_REQ_SEND, task.remoteId);
    if (!ClientCore::syncInvoke(clientCore_, frame)) {
        qCritical() << QString(tr("返回发送请求失败：%1")).arg(info.msg);
        sendTask_->state = TaskState::STATE_NONE;
        sendTask_->file.close();
        return;
    }
    sendTask_->state = TaskState::STATE_RUNNING;
    sendTask_->totalSize = info.size;
    sendTask_->tranSize = 0;
    sendTask_->task = task;
}

void FileTrans::ReqDownFile(const TransTask& task)
{
    // start
    InfoMsg info;
    info.toPath = task.localPath;
    info.fromPath = task.remotePath;

    downTask_->task = task;
    downTask_->totalSize = 0;

    // recv
    if (!Util::DirExist(info.toPath, false)) {
        QDir dir;
        if (!dir.mkpath(info.toPath)) {
            info.msg = QString(tr("创建目录失败：%1")).arg(info.toPath);
            qCritical() << info.msg;
            auto f = clientCore_->GetBuffer(info, FBT_CLI_CANOT_SEND, task.remoteId);
            if (!ClientCore::syncInvoke(clientCore_, f)) {
                qCritical() << QString(tr("%1 回复 %2 失败。")).arg(info.msg, f->fid);
            }
            return;
        }
        qInfo() << QString(tr("目录 %1 不存在，已自动创建。")).arg(info.toPath);
    }

    downTask_->file.setFileName(Util::Get2FilePath(task.remotePath, task.localPath));
    if (!downTask_->file.open(QIODevice::WriteOnly)) {
        qCritical() << QString(tr("打开文件 [%1] 失败。")).arg(downTask_->file.fileName());
        downTask_->state = TaskState::STATE_NONE;
        return;
    }
    auto frame = clientCore_->GetBuffer(info, FBT_CLI_REQ_DOWN, task.remoteId);
    if (!ClientCore::syncInvoke(clientCore_, frame)) {
        qCritical() << QString(tr("返回发送请求失败：%1")).arg(info.msg);
        downTask_->state = TaskState::STATE_NONE;
        downTask_->file.close();
        return;
    }
    downTask_->state = TaskState::STATE_RUNNING;
}

qint32 FileTrans::GetSendProgress()
{
    if (sendTask_->state == TaskState::STATE_FINISH) {
        sendTask_->state = TaskState::STATE_NONE;
        return 100;
    }
    if (sendTask_->state != TaskState::STATE_RUNNING) {
        return -1;
    }

    // wait file state ok.
    double per = (sendTask_->tranSize * 100.0) / sendTask_->totalSize;
    qint32 rt = static_cast<qint32>(per);
    if (rt >= 100) {
        return 99;
    }
    return rt;
}

qint32 FileTrans::GetDownProgress()
{
    if (downTask_->state == TaskState::STATE_FINISH) {
        downTask_->state = TaskState::STATE_NONE;
        return 100;
    }
    if (downTask_->state != TaskState::STATE_RUNNING) {
        return -1;
    }
    if (downTask_->totalSize == 0) {
        return 0;
    }
    // wait file state ok.
    double per = (downTask_->tranSize * 100.0) / downTask_->totalSize;
    qint32 rt = static_cast<qint32>(per);
    if (rt >= 100) {
        return 99;
    }
    return rt;
}

void FileTrans::RegisterSignal()
{
    connect(clientCore_, &ClientCore::sigReqSend, this, [this](QSharedPointer<FrameBuffer> frame) { fbtReqSend(frame); });
    connect(clientCore_, &ClientCore::sigReqDown, this, [this](QSharedPointer<FrameBuffer> frame) { fbtReqDown(frame); });
    connect(clientCore_, &ClientCore::sigTransDone, this, [this](QSharedPointer<FrameBuffer> frame) { fbtTransDone(frame); });
    connect(clientCore_, &ClientCore::sigCanSend, this, [this](QSharedPointer<FrameBuffer> frame) { fbtCanSend(frame); });
    connect(clientCore_, &ClientCore::sigCanotSend, this, [this](QSharedPointer<FrameBuffer> frame) { fbtCanotSend(frame); });
    connect(clientCore_, &ClientCore::sigCanotDown, this, [this](QSharedPointer<FrameBuffer> frame) { fbtCanotDown(frame); });
    connect(clientCore_, &ClientCore::sigCanDown, this, [this](QSharedPointer<FrameBuffer> frame) { fbtCanDown(frame); });
    connect(clientCore_, &ClientCore::sigFileBuffer, this, [this](QSharedPointer<FrameBuffer> frame) { fbtFileBuffer(frame); });
    connect(clientCore_, &ClientCore::sigFileInfo, this, [this](QSharedPointer<FrameBuffer> frame) { fbtFileInfo(frame); });
    connect(clientCore_, &ClientCore::sigTransFailed, this, [this](QSharedPointer<FrameBuffer> frame) { fbtTransFailed(frame); });
    connect(clientCore_, &ClientCore::sigOffline, this, [this](QSharedPointer<FrameBuffer> frame) { fbtInterrupt(frame); });
    connect(clientCore_, &ClientCore::sigTransInterrupt, this,
            [this](QSharedPointer<FrameBuffer> frame) { fbtInterrupt(frame); });
}

// The other party requests to send, prepare to receive.
void FileTrans::fbtReqSend(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qInfo() << QString(tr("%1 请求发送 %2 到 %3")).arg(frame->fid).arg(info.fromPath, info.toPath);

    // judge is same client's same file.

    // recv is single thread recv, judge idle
    if (downTask_->state == TaskState::STATE_RUNNING) {
        info.msg = QString(tr("繁忙......"));
        auto f = clientCore_->GetBuffer(info, FBT_CLI_CANOT_SEND, frame->fid);
        ClientCore::syncInvoke(clientCore_, f);
        return;
    }

    // recv
    if (!Util::DirExist(info.toPath, false)) {
        QDir dir;
        if (!dir.mkpath(info.toPath)) {
            info.msg = QString(tr("创建目录失败：%1")).arg(info.toPath);
            qCritical() << info.msg;
            auto f = clientCore_->GetBuffer(info, FBT_CLI_CANOT_SEND, frame->fid);
            if (!ClientCore::syncInvoke(clientCore_, f)) {
                qCritical() << QString(tr("%1 回复 %2 失败。")).arg(info.msg, f->fid);
            }
            return;
        }
        qInfo() << QString(tr("目录 %1 不存在，已自动创建。")).arg(info.toPath);
    }

    auto newerPath = Util::Get2FilePath(info.fromPath, info.toPath);
    downTask_->file.setFileName(newerPath);
    if (!downTask_->file.open(QIODevice::WriteOnly)) {
        info.msg = QString(tr("打卡文件失败： %1")).arg(newerPath);
        qCritical() << info.msg;
        auto f = clientCore_->GetBuffer(info, FBT_CLI_CANOT_SEND, frame->fid);
        if (!ClientCore::syncInvoke(clientCore_, f)) {
            qCritical() << QString(tr("打开接收文件 %1 失败，回复 %2 失败。")).arg(info.msg, f->fid);
            downTask_->file.close();
        }
        return;
    }

    downTask_->totalSize = info.size;
    downTask_->tranSize = 0;
    downTask_->permission = info.permissions;

    info.msg = QString(tr("打开待接收文件成功：%1")).arg(newerPath);
    qInfo() << info.msg;
    auto f = clientCore_->GetBuffer(info, FBT_CLI_CAN_SEND, frame->fid);
    if (!ClientCore::syncInvoke(clientCore_, f)) {
        qCritical() << QString(tr("打开接收文件 %1 成功, 但是回复 %2 失败。")).arg(info.msg, frame->fid);
        downTask_->file.close();
        return;
    }

    // 需要记录对方的ID，用于心跳检测
    clientCore_->pushID(frame->fid);
    downTask_->state = TaskState::STATE_RUNNING;
}

// The other party requests to download, prepare to send.
void FileTrans::fbtReqDown(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    // send
    auto doTask = QSharedPointer<DoTransTask>::create();
    doTask->file.setFileName(info.fromPath);
    if (!doTask->file.open(QIODevice::ReadOnly)) {
        qCritical() << QString(tr("打开文件失败：%1")).arg(info.fromPath);
        return;
    }

    QFileInfo fileInfo(info.fromPath);
    if (fileInfo.exists()) {
        qint64 size = fileInfo.size();
        info.permissions = static_cast<quint32>(fileInfo.permissions());
        info.size = size;
    } else {
        qCritical() << QString(tr("文件 [%1] 不存在。")).arg(info.fromPath);
        doTask->file.close();
        return;
    }

    // reply fileinfo
    auto f = clientCore_->GetBuffer(info, FBT_CLI_FILE_INFO, frame->fid);
    if (!ClientCore::syncInvoke(clientCore_, f)) {
        qCritical() << QString(tr("发送 %1 信息失败。")).arg(info.fromPath);
        doTask->file.close();
        return;
    }
    doTask->task.isUpload = true;
    doTask->task.localPath = info.fromPath;
    doTask->task.remoteId = frame->fid;

    // 需要记录对方的ID，用于心跳检测
    clientCore_->pushID(frame->fid);

    SendFile(doTask);
}

void FileTrans::fbtTransDone(QSharedPointer<FrameBuffer> frame)
{
    auto info = infoUnpack<InfoMsg>(frame->data);
    if (downTask_->file.isOpen()) {
        downTask_->file.setPermissions(static_cast<QFileDevice::Permissions>(downTask_->permission));
        downTask_->file.close();
        downTask_->state = TaskState::STATE_FINISH;
        info.msg = QString(tr("接收文件：%1 成功。")).arg(downTask_->file.fileName());
        qInfo() << info.msg;
        return;
    }
    qCritical() << QString(tr("成功收到了信号：%1 但是文件未打开。")).arg(info.msg);
}

// the other party indicates that can download, ready to receive the file.
void FileTrans::fbtCanDown(QSharedPointer<FrameBuffer> frame)
{
    // ready to recv file.
    auto info = infoUnpack<InfoMsg>(frame->data);
    downTask_->permission = info.permissions;
    downTask_->totalSize = info.size;
    downTask_->tranSize = 0;
    qDebug() << QString(tr("Can Down trans file:%1.")).arg(info.fromPath);
}

void FileTrans::fbtCanotDown(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qCritical() << QString(tr("请求发送文件 %1 失败，原因：%2")).arg(info.fromPath, info.msg);
}

void FileTrans::fbtFileBuffer(QSharedPointer<FrameBuffer> frame)
{
    if (downTask_->state != TaskState::STATE_RUNNING) {
        return;
    }
    // For the sake of efficiency, not verify the legality of the file
    auto ws = downTask_->file.write(frame->data.constData(), frame->data.size());
    if (ws != frame->data.size()) {
        downTask_->state = TaskState::STATE_FAILED;
        InfoMsg info;
        info.msg = downTask_->file.errorString();
        auto f = clientCore_->GetBuffer(info, FBT_CLI_TRANS_FAILED, frame->fid);
        ClientCore::syncInvoke(clientCore_, f);
        downTask_->file.close();
    }
    downTask_->tranSize += ws;
}

void FileTrans::fbtFileInfo(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qInfo() << QString(tr("准备接收文件的大小：%1，权限：%2")).arg(info.size).arg(info.permissions);
    downTask_->totalSize = info.size;
    downTask_->tranSize = 0;
    downTask_->permission = info.permissions;
}

void FileTrans::fbtCanotSend(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qCritical() << QString(tr("请求文件 %1 失败，原因：%2")).arg(info.fromPath, info.msg);
    if (sendTask_->file.isOpen()) {
        sendTask_->file.close();
    }
}

void FileTrans::fbtCanSend(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qInfo() << QString(tr("开始发送 %1 到 %2")).arg(info.fromPath, frame->fid);
    SendFile(sendTask_);
}

void FileTrans::fbtTransFailed(QSharedPointer<FrameBuffer> frame)
{
    qCritical() << QString(tr("传输文件 %1 失败。")).arg(downTask_->file.fileName());
    if (downTask_->file.isOpen()) {
        downTask_->file.close();
    }
    downTask_->state = TaskState::STATE_FAILED;
}

void FileTrans::Interrupt(bool notic)
{
    if (downTask_->state == TaskState::STATE_RUNNING) {
        qWarning() << QString(tr("传输文件 %1 中断。")).arg(downTask_->file.fileName());
        downTask_->file.close();

        if (notic) {
            InfoMsg info;
            info.msg = QString(tr("传输文件 %1 主动中断。")).arg(downTask_->file.fileName());
            auto f = clientCore_->GetBuffer(info, FBT_CLI_TRANS_INTERRUPT, downTask_->task.remoteId);
            if (!ClientCore::syncInvoke(clientCore_, f)) {
                qCritical() << QString(tr("发送 %1 信息给 %2 失败。")).arg(info.msg).arg(downTask_->task.remoteId);
            }
        }

        downTask_->state = TaskState::STATE_NONE;
    }
    if (sendTask_->state == TaskState::STATE_RUNNING) {
        qWarning() << QString(tr("传输文件 %1 中断。")).arg(sendTask_->file.fileName());
        sendTask_->file.close();

        InfoMsg info;
        info.msg = QString(tr("传输文件 %1 主动中断。")).arg(sendTask_->file.fileName());
        auto f = clientCore_->GetBuffer(info, FBT_CLI_TRANS_INTERRUPT, sendTask_->task.remoteId);
        if (!ClientCore::syncInvoke(clientCore_, f)) {
            qCritical() << QString(tr("发送 %1 信息给 %2 失败。")).arg(info.msg).arg(sendTask_->task.remoteId);
        }

        sendTask_->state = TaskState::STATE_NONE;
    }
}

void FileTrans::fbtInterrupt(QSharedPointer<FrameBuffer> frame)
{
    Interrupt(false);
}

void FileTrans::SendFile(const QSharedPointer<DoTransTask>& task)
{
    auto* sendThread = new SendThread(clientCore_);
    sendThread->setTask(task);
    connect(clientCore_, &ClientCore::sigFlowBack, sendThread, &SendThread::fbtFlowBack);

    QMutexLocker locker(&sthMut_);
    upTasks_[task->task.localId] = sendThread;
    sendThread->run();
}

SendThread::SendThread(ClientCore* clientCore) : cliCore_(clientCore)
{
}

// 发送速度控制
void SendThread::fbtFlowBack(QSharedPointer<FrameBuffer> frame)
{
    auto msg = infoUnpack<InfoMsg>(frame->data);
    delay_ = msg.mark * BLOCK_LEVEL_MULTIPLE;
}

void SendThread::run()
{
    // task's file shoule be already opened.
    isSuccess_ = true;
    delay_ = 0;
    bool invokeSuccess = false;
    while (!task_->file.atEnd()) {
        auto frame = QSharedPointer<FrameBuffer>::create();
        frame->tid = task_->task.remoteId;
        frame->type = FBT_CLI_FILE_BUFFER;
        frame->data.resize(CHUNK_BUF_SIZE);

        auto br = task_->file.read(frame->data.data(), CHUNK_BUF_SIZE);
        if (br == -1) {
            qCritical() << QString(tr("读取失败： %1")).arg(task_->file.errorString());
            isSuccess_ = false;
            break;
        }
        frame->data.resize(br);
        if (delay_ > 0) {
            QThread::msleep(delay_);
        }
        invokeSuccess = QMetaObject::invokeMethod(cliCore_, "SendFrame", Qt::BlockingQueuedConnection,
                                                  Q_RETURN_ARG(bool, isSuccess_), Q_ARG(QSharedPointer<FrameBuffer>, frame));
        if (!invokeSuccess || !isSuccess_) {
            qCritical() << QString(tr("向 %1 发送失败.")).arg(task_->task.remoteId);
            break;
        }
        task_->tranSize += frame->data.size();
        // 关键点：这里不调用，无法中途收到别人发的数据。
        QCoreApplication::processEvents();
    }
    qInfo() << QString(tr("结束发送文件：%1")).arg(task_->file.fileName());

    // 不是Open表示被别人关闭了，就不发送结束信号了。
    if (task_->file.isOpen()) {
        InfoMsg info;
        auto f = cliCore_->GetBuffer(info, FBT_CLI_TRANS_DONE, task_->task.remoteId);
        ClientCore::syncInvoke(cliCore_, f);
        task_->file.close();
        task_->state = TaskState::STATE_FINISH;
    }
}

void SendThread::setTask(const QSharedPointer<DoTransTask>& task)
{
    task_ = task;
}
