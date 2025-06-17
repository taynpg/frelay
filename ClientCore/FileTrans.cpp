#include "FileTrans.h"

FileTrans::FileTrans(ClientCore* clientCore) : clientCore_(clientCore)
{
    RegisterFrameCall();
}

void FileTrans::SetTasks(const QVector<TransTask>& tasks)
{
    localTasks_ = tasks;
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
    qInfo() << QString(tr("%1 req send: %2 to %3")).arg(frame->fid).arg(info.fromPath).arg(info.toPath);

    // judge is same client's same file.

    // recv is single thread recv, judge idle
    if (downTask_.state == TaskState::STATE_RUNNING) {
        info.msg = QString(tr("busy"));
        clientCore_->Send<InfoMsg>(info, FBT_CLI_CANOT_SEND, frame->fid);
        return;
    }

    // recv
    auto newerPath = Util::Get2FilePath(info.fromPath, info.toPath);
    downTask_.file.setFileName(newerPath);
    if (!downTask_.file.open(QIODevice::WriteOnly)) {
        info.msg = QString(tr("open file failed: %1")).arg(newerPath);
        qCritical() << info.msg;
        if (!clientCore_->Send<InfoMsg>(info, FBT_CLI_CANOT_SEND, frame->fid)) {
            qCritical() << QString(tr("open recv file:%2 failed, and reply %2 failed.")).arg(info.msg).arg(frame->fid);
            downTask_.file.close();
            return;
        }
        return;
    }

    downTask_.totalSize = info.size;
    downTask_.tranSize = 0;
    downTask_.permission = info.permissions;

    info.msg = QString(tr("open recv file success: %1")).arg(newerPath);
    if (!clientCore_->Send<InfoMsg>(info, FBT_CLI_CAN_SEND, frame->fid)) {
        qCritical() << QString(tr("open recv file:%2 success, but reply %2 failed.")).arg(info.msg).arg(frame->fid);
        downTask_.file.close();
        return;
    }
    downTask_.state = TaskState::STATE_RUNNING;
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
    if (downTask_.file.isOpen()) {
        downTask_.file.close();
        downTask_.state = TaskState::STATE_FINISH;
        qInfo() << QString(tr("recv file:%1 success.")).arg(downTask_.file.fileName());
        clientCore_->Send<InfoMsg>(info, FBT_CLI_CAN_DOWN, frame->fid);
        return;
    }
    qCritical() << QString(tr("recv file:%1 done sigal, but file not opened.")).arg(info.msg);
}

void FileTrans::fbtCanDown(QSharedPointer<FrameBuffer> frame)
{
    // ready to send
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    auto doTask = QSharedPointer<DoTransTask>::create();
    doTask->file.setFileName(info.fromPath);
    if (!doTask->file.open(QIODevice::ReadOnly)) {
        qCritical() << QString(tr("open file failed: %1")).arg(info.fromPath);
        return;
    }
    doTask->task.isUpload = true;
    doTask->task.localPath = "";
    doTask->task.remoteId = frame->fid;
    SendFile(doTask);
}

void FileTrans::fbtCanotDown(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qCritical() << QString(tr("request send file:%1 failed. reason:%2")).arg(info.fromPath).arg(info.msg);
}

void FileTrans::fbtFileBuffer(QSharedPointer<FrameBuffer> frame)
{
    if (downTask_.state != TaskState::STATE_RUNNING) {
        return;
    }
    // For the sake of efficiency, not verify the legality of the file
    auto ws = downTask_.file.write(frame->data.constData(), frame->data.size());
    if (ws != frame->data.size()) {
        downTask_.state = TaskState::STATE_FAILED;
        InfoMsg info;
        info.msg = downTask_.file.errorString();
        clientCore_->Send<InfoMsg>(info, FBT_CLI_TRANS_FAILED, frame->fid);
    }
}

void FileTrans::fbtCanotSend(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qCritical() << QString(tr("request file:%1 failed. reason:%2")).arg(info.fromPath).arg(info.msg);
}

void FileTrans::fbtCanSend(QSharedPointer<FrameBuffer> frame)
{
}

void FileTrans::fbtTransFailed(QSharedPointer<FrameBuffer> frame)
{
    qCritical() << QString(tr("trans file:%1 failed.")).arg(downTask_.file.fileName());
    if (downTask_.file.isOpen()) {
        downTask_.file.close();
    }
    downTask_.state = TaskState::STATE_FAILED;
}

void FileTrans::SendFile(const QSharedPointer<DoTransTask>& task)
{
    auto* sendThread = new SendThread(clientCore_);
    sendThread->setTask(task);
    QMutexLocker locker(&sthMut_);
    sendThreads_.push_back(sendThread);
    sendThread->run();
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

    bool suc = true;
    while (!task_->file.atEnd()) {
        frame->data.resize(CHUNK_BUF_SIZE);
        auto br = task_->file.read(frame->data.data(), CHUNK_BUF_SIZE);
        if (br == -1) {
            qCritical() << QString(tr("read file failed: %1")).arg(task_->file.errorString());
            suc = false;
            break;
        }
        frame->data.resize(br);
        if (!cliCore_->Send(frame)) {
            qCritical() << QString(tr("send to %1 file failed.")).arg(task_->task.remoteId);
            suc = false;
            break;
        }
    }

    if (!suc) {
        task_->file.close();
    }
}

void SendThread::setTask(const QSharedPointer<DoTransTask>& task)
{
    task_ = task;
}