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
    clientCore_->SetFrameCall(FBT_CLI_REQ_RECV, [this](QSharedPointer<FrameBuffer> frame) { fbtReqRecv(frame); });
    clientCore_->SetFrameCall(FBT_CLI_TRANS_DONE, [this](QSharedPointer<FrameBuffer> frame) { fbtTransDone(frame); });
    clientCore_->SetFrameCall(FBT_CLI_ANSSEND_SUCCESS, [this](QSharedPointer<FrameBuffer> frame) { fbtAnsSendSuccess(frame); });
    clientCore_->SetFrameCall(FBT_CLI_ANSSEND_FAILED, [this](QSharedPointer<FrameBuffer> frame) { fbtAnsSendFailed(frame); });
    clientCore_->SetFrameCall(FBT_CLI_ANSRECV_FAILED, [this](QSharedPointer<FrameBuffer> frame) { fbtAnsRecvFailed(frame); });
    clientCore_->SetFrameCall(FBT_CLI_ANSRECV_SUCCESS, [this](QSharedPointer<FrameBuffer> frame) { fbtAnsRecvSuccess(frame); });
    clientCore_->SetFrameCall(FBT_CLI_FILETRANS, [this](QSharedPointer<FrameBuffer> frame) { fbtFileTrans(frame); });
    clientCore_->SetFrameCall(FBT_CLI_FILETRANS_FAILED, [this](QSharedPointer<FrameBuffer> frame) { fbtFileTransFailed(frame); });
}

void FileTrans::fbtReqSend(QSharedPointer<FrameBuffer> frame)
{
    // judget is same client's same file.

    // send
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    auto doTask = QSharedPointer<DoTransTask>::create();
    doTask->file.setFileName(info.path);
    if (!doTask->file.open(QIODevice::ReadOnly)) {
        qCritical() << QString(tr("open file failed: %1")).arg(info.path);
        return;
    }
    doTask->task.isUpload = true;
    doTask->task.localPath = info.path;
    doTask->task.remoteId = frame->fid;
    SendFile(doTask);
}

void FileTrans::fbtReqRecv(QSharedPointer<FrameBuffer> frame)
{
    // recv is single thread recv.

    // judge idle

    // reply msg

    // recv
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    downTask_.file.setFileName(info.path);
    if (!downTask_.file.open(QIODevice::WriteOnly)) {
        info.msg = QString(tr("open file failed: %1")).arg(info.path);
        qCritical() << info.msg;
        if (!clientCore_->Send<InfoMsg>(info, FBT_CLI_ANSRECV_FAILED, frame->fid)) {
            qCritical() << QString(tr("open recv file:%2 failed, and reply %2 failed.")).arg(info.msg).arg(frame->fid);
            downTask_.file.close();
            return;
        }
        return;
    }
    info.msg = QString(tr("open recv file success: %1")).arg(info.path);
    if (!clientCore_->Send<InfoMsg>(info, FBT_CLI_ANSRECV_SUCCESS, frame->fid)) {
        qCritical() << QString(tr("open recv file:%2 success, but reply %2 failed.")).arg(info.msg).arg(frame->fid);
        downTask_.file.close();
        return;
    }
    downTask_.state = TaskState::STATE_RUNNING;
}

void FileTrans::fbtTransDone(QSharedPointer<FrameBuffer> frame)
{
    auto info = infoUnpack<InfoMsg>(frame->data);
    if (downTask_.file.isOpen()) {
        downTask_.file.close();
        downTask_.state = TaskState::STATE_FINISH;
        qInfo() << QString(tr("recv file:%1 success.")).arg(downTask_.file.fileName());
        clientCore_->Send<InfoMsg>(info, FBT_CLI_ANSRECV_SUCCESS, frame->fid);
        return;
    }
    qCritical() << QString(tr("recv file:%1 done sigal, but file not opened.")).arg(info.msg);
}

void FileTrans::fbtAnsRecvSuccess(QSharedPointer<FrameBuffer> frame)
{
    // ready to send
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    auto doTask = QSharedPointer<DoTransTask>::create();
    doTask->file.setFileName(info.path);
    if (!doTask->file.open(QIODevice::ReadOnly)) {
        qCritical() << QString(tr("open file failed: %1")).arg(info.path);
        return;
    }
    doTask->task.isUpload = true;
    doTask->task.localPath = info.path;
    doTask->task.remoteId = frame->fid;
    SendFile(doTask);
}

void FileTrans::fbtAnsRecvFailed(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    qCritical() << QString(tr("request send file:%1 failed. reason:%2")).arg(info.path).arg(info.msg);
}

void FileTrans::fbtFileTrans(QSharedPointer<FrameBuffer> frame)
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
        clientCore_->Send<InfoMsg>(info, FBT_CLI_FILETRANS_FAILED, frame->fid);
    }
}

void FileTrans::fbtAnsSendFailed(QSharedPointer<FrameBuffer> frame)
{
}

void FileTrans::fbtAnsSendSuccess(QSharedPointer<FrameBuffer> frame)
{
}

void FileTrans::fbtFileTransFailed(QSharedPointer<FrameBuffer> frame)
{
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
    frame->type = FBT_CLI_FILETRANS;

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