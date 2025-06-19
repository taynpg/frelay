#include "FileTrans.h"

#include <QFileInfo>

FileTrans::FileTrans(ClientCore* clientCore) : clientCore_(clientCore)
{
    RegisterFrameCall();
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
    // TODO: check if running...

    // start
    InfoMsg info;
    info.toPath = task.remotePath;
    info.fromPath = task.localPath;

    sendTask_->file.setFileName(info.fromPath);
    if (!sendTask_->file.open(QIODevice::ReadOnly)) {
        qCritical() << QString(tr("open file [%1] failed.")).arg(info.fromPath);
        sendTask_->state = TaskState::STATE_NONE;
        return;
    }

    QFileInfo fileInfo(info.fromPath);
    if (fileInfo.exists()) {
        qint64 size = fileInfo.size();
        info.permissions = static_cast<quint32>(fileInfo.permissions());
        info.size = size;
    } else {
        qCritical() << QString(tr("File [%1] not exit.")).arg(info.fromPath);
        sendTask_->file.close();
        return;
    }

    if (!clientCore_->Send<InfoMsg>(info, FBT_CLI_REQ_SEND, task.remoteId)) {
        qCritical() << QString(tr("send req send failed: %1")).arg(info.msg);
        sendTask_->state = TaskState::STATE_NONE;
        sendTask_->file.close();
        return;
    }
    sendTask_->state = TaskState::STATE_RUNNING;
    sendTask_->totalSize = info.size;
    sendTask_->tranSize = 0;
}

void FileTrans::ReqDownFile(const TransTask& task)
{
    // TODO: check if running...

    // start
    InfoMsg info;
    info.toPath = task.localPath;
    info.fromPath = task.remotePath;

    downTask_->file.setFileName(Util::Get2FilePath(task.remotePath, task.localPath));
    if (!downTask_->file.open(QIODevice::WriteOnly)) {
        qCritical() << QString(tr("open file [%1] failed.")).arg(downTask_->file.fileName());
        downTask_->state = TaskState::STATE_NONE;
        return;
    }
    if (!clientCore_->Send<InfoMsg>(info, FBT_CLI_REQ_DOWN, task.remoteId)) {
        qCritical() << QString(tr("send req send failed: %1")).arg(info.msg);
        sendTask_->state = TaskState::STATE_NONE;
        sendTask_->file.close();
        return;
    }
    sendTask_->state = TaskState::STATE_RUNNING;
}

qint32 FileTrans::GetSendProgress()
{
    if (sendTask_->state != TaskState::STATE_RUNNING) {
        return -1;
    }

    double per = (sendTask_->tranSize * 100.0) / sendTask_->totalSize;
    return per;
}

qint32 FileTrans::GetDownProgress()
{
    if (downTask_->state != TaskState::STATE_RUNNING) {
        return -1;
    }

    double per = (downTask_->tranSize * 100.0) / downTask_->totalSize;
    return per;
}

void FileTrans::RegisterFrameCall()
{
    clientCore_->SetFrameCall(FBT_CLI_REQ_SEND, [this](QSharedPointer<FrameBuffer> frame) { fbtReqSend(frame); });
    clientCore_->SetFrameCall(FBT_CLI_REQ_DOWN, [this](QSharedPointer<FrameBuffer> frame) { fbtReqDown(frame); });
    clientCore_->SetFrameCall(FBT_CLI_TRANS_DONE, [this](QSharedPointer<FrameBuffer> frame) { fbtTransDone(frame); });
    clientCore_->SetFrameCall(FBT_CLI_CAN_SEND, [this](QSharedPointer<FrameBuffer> frame) { fbtCanSend(frame); });
    clientCore_->SetFrameCall(FBT_CLI_CANOT_SEND, [this](QSharedPointer<FrameBuffer> frame) { fbtCanotSend(frame); });
    clientCore_->SetFrameCall(FBT_CLI_CANOT_DOWN, [this](QSharedPointer<FrameBuffer> frame) { fbtCanotDown(frame); });
    clientCore_->SetFrameCall(FBT_CLI_CAN_DOWN, [this](QSharedPointer<FrameBuffer> frame) { fbtCanDown(frame); });
    clientCore_->SetFrameCall(FBT_CLI_FILE_BUFFER, [this](QSharedPointer<FrameBuffer> frame) { fbtFileBuffer(frame); });
    clientCore_->SetFrameCall(FBT_CLI_TRANS_FAILED, [this](QSharedPointer<FrameBuffer> frame) { fbtTransFailed(frame); });
}

// The other party requests to send, prepare to receive.
void FileTrans::fbtReqSend(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qInfo() << QString(tr("%1 req send: %2 to %3")).arg(frame->fid).arg(info.fromPath, info.toPath);

    // judge is same client's same file.

    // recv is single thread recv, judge idle
    if (downTask_->state == TaskState::STATE_RUNNING) {
        info.msg = QString(tr("busy..."));
        clientCore_->Send<InfoMsg>(info, FBT_CLI_CANOT_SEND, frame->fid);
        return;
    }

    // recv
    auto newerPath = Util::Get2FilePath(info.fromPath, info.toPath);
    downTask_->file.setFileName(newerPath);
    if (!downTask_->file.open(QIODevice::WriteOnly)) {
        info.msg = QString(tr("open file failed: %1")).arg(newerPath);
        qCritical() << info.msg;
        if (!clientCore_->Send<InfoMsg>(info, FBT_CLI_CANOT_SEND, frame->fid)) {
            qCritical() << QString(tr("open recv file:%2 failed, and reply %2 failed.")).arg(info.msg, frame->fid);
            downTask_->file.close();
            return;
        }
        return;
    }

    downTask_->totalSize = info.size;
    downTask_->tranSize = 0;
    downTask_->permission = info.permissions;

    info.msg = QString(tr("open recv file success: %1")).arg(newerPath);
    auto f = clientCore_->GetBuffer(info, FBT_CLI_CAN_SEND, frame->fid);
    auto sendRet = ClientCore::asyncInvoke(clientCore_, [this, f]() { return clientCore_->Send(f); });
    if (!sendRet) {
        qCritical() << QString(tr("open recv file:%2 success, but reply %2 failed.")).arg(info.msg, frame->fid);
        downTask_->file.close();
        return;
    }
    downTask_->state = TaskState::STATE_RUNNING;
}

// The other party requests to download, prepare to send.
void FileTrans::fbtReqDown(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    // judge is same client's same file.

    // judge if file exits etc.

    // send
    auto doTask = QSharedPointer<DoTransTask>::create();
    doTask->file.setFileName(info.fromPath);
    if (!doTask->file.open(QIODevice::ReadOnly)) {
        qCritical() << QString(tr("open file failed: %1")).arg(info.fromPath);
        return;
    }
    doTask->task.isUpload = true;
    doTask->task.localPath = info.fromPath;
    doTask->task.remoteId = frame->fid;
    SendFile(doTask);
}

void FileTrans::fbtTransDone(QSharedPointer<FrameBuffer> frame)
{
    auto info = infoUnpack<InfoMsg>(frame->data);
    if (downTask_->file.isOpen()) {
        downTask_->file.close();
        downTask_->state = TaskState::STATE_FINISH;
        qInfo() << QString(tr("recv file:%1 success.")).arg(downTask_->file.fileName());
        clientCore_->Send<InfoMsg>(info, FBT_CLI_CAN_DOWN, frame->fid);
        return;
    }
    qCritical() << QString(tr("recv file:%1 done sigal, but file not opened.")).arg(info.msg);
}

// the other party indicates that can download, ready to receive the file.
void FileTrans::fbtCanDown(QSharedPointer<FrameBuffer> frame)
{
    // ready to recv file.
    auto info = infoUnpack<InfoMsg>(frame->data);
    downTask_->permission = info.permissions;
    downTask_->totalSize = info.size;
    downTask_->tranSize = 0;
    qDebug() << QString(tr("start trans file:%1.")).arg(info.fromPath);
}

void FileTrans::fbtCanotDown(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qCritical() << QString(tr("request send file:%1 failed. reason:%2")).arg(info.fromPath, info.msg);
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
        clientCore_->Send<InfoMsg>(info, FBT_CLI_TRANS_FAILED, frame->fid);
        downTask_->file.close();
    }
    downTask_->tranSize += ws;
}

void FileTrans::fbtCanotSend(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qCritical() << QString(tr("request file:%1 failed. reason:%2")).arg(info.fromPath, info.msg);
    if (sendTask_->file.isOpen()) {
        sendTask_->file.close();
    }
}

void FileTrans::fbtCanSend(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qInfo() << QString(tr("start trans file:%1 to %2")).arg(info.fromPath, frame->fid);
    SendFile(sendTask_);
}

void FileTrans::fbtTransFailed(QSharedPointer<FrameBuffer> frame)
{
    qCritical() << QString(tr("trans file:%1 failed.")).arg(downTask_->file.fileName());
    if (downTask_->file.isOpen()) {
        downTask_->file.close();
    }
    downTask_->state = TaskState::STATE_FAILED;
}

void FileTrans::SendFile(const QSharedPointer<DoTransTask>& task)
{
    auto* sendThread = new SendThread(clientCore_);
    sendThread->setTask(task);
    QMutexLocker locker(&sthMut_);
    // TODO: check if already exist
    upTasks_[task->task.localId] = sendThread;
    sendThread->run();
}

QFuture<bool> FileTrans::sendFrameAsync(const QSharedPointer<FrameBuffer>& frame)
{
    auto promise = QSharedPointer<QPromise<bool>>::create();
    QFuture<bool> future = promise->future();
    QMetaObject::invokeMethod(
        clientCore_,
        [this, frame, promise]() mutable {
            bool ret = clientCore_->Send(frame);
            promise->addResult(ret);
            promise->finish();
        },
        Qt::QueuedConnection);
    return future;
}

SendThread::SendThread(ClientCore* clientCore) : cliCore_(clientCore)
{
}

void SendThread::run()
{
    // task's file shoule be already opened.
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->tid = task_->task.remoteId;
    frame->type = FBT_CLI_FILE_BUFFER;
    frame->call = [this](QSharedPointer<FrameBuffer> frame) { sendCall(frame); };

    isSuccess_ = true;
    while (!task_->file.atEnd()) {
        frame->data.resize(CHUNK_BUF_SIZE);
        auto br = task_->file.read(frame->data.data(), CHUNK_BUF_SIZE);
        if (br == -1) {
            qCritical() << QString(tr("read file failed: %1")).arg(task_->file.errorString());
            isSuccess_ = false;
            break;
        }
        frame->data.resize(br);

        while (curSendCount_ >= MAX_SEND_TASK) {
            QThread::msleep(1);
            // shoule add abort action mark.
        }

        QMetaObject::invokeMethod(this, [this, frame] {
            frame->sendRet = cliCore_->Send(frame);
            if (frame->call) {
                frame->call(frame);
            }
        });
        ++curSendCount_;

        if (!isSuccess_) {
            qCritical() << QString(tr("send to %1 file failed.")).arg(task_->task.remoteId);
            break;
        }
    }
    task_->file.close();
}

void SendThread::setTask(const QSharedPointer<DoTransTask>& task)
{
    task_ = task;
}

void SendThread::sendCall(QSharedPointer<FrameBuffer> frame)
{
    if (frame->sendRet) {
        --curSendCount_;
        task_->tranSize += frame->data.size();
    } else {
        isSuccess_ = false;
    }
}
