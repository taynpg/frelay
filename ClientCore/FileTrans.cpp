#include "FileTrans.h"

FileTrans::FileTrans(ClientCore* clientCore) : clientCore_(clientCore)
{
}

void FileTrans::SetTasks(const QVector<TransTask>& tasks)
{
    tasks_ = tasks;
}

void FileTrans::RegisterFrameCall()
{
    clientCore_->SetFrameCall(FBT_CLI_REQ_SEND, [this](QSharedPointer<FrameBuffer> frame) { fbtReqSend(frame); });
}

void FileTrans::fbtReqSend(QSharedPointer<FrameBuffer> frame)
{
}
