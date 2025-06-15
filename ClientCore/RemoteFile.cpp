#include "RemoteFile.h"

#include <InfoPack.hpp>
#include "LocalFile.h"

void RemoteFile::setClientCore(ClientCore* cliCore)
{
    cliCore_ = cliCore;
    cliCore_->SetPathCall(pathCall_);
    cliCore_->SetFileCall(fileCall_);
}

bool RemoteFile::GetHome()
{
    InfoMsg info;
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->data = infoPack(info);
    frame->type = FBT_CLI_ASK_HOME;
    frame->tid = cliCore_->GetRemoteID();
    return cliCore_->Send(frame);
}

bool RemoteFile::GetDirFile(const QString& dir)
{
    InfoMsg info;
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->data = infoPack(info);
    frame->type = FBT_CLI_ASK_DIRFILE;
    frame->tid = cliCore_->GetRemoteID();
    return cliCore_->Send(frame);
}