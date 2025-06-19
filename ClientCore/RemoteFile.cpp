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
    auto frame = cliCore_->GetBuffer(info, FBT_CLI_ASK_HOME, cliCore_->GetRemoteID());
    return ClientCore::asyncInvoke(cliCore_, [this, frame]() { return cliCore_->Send(frame); });
}

bool RemoteFile::GetDirFile(const QString& dir)
{
    InfoMsg info;
    auto frame = cliCore_->GetBuffer(info, FBT_CLI_ASK_DIRFILE, cliCore_->GetRemoteID());
    return ClientCore::asyncInvoke(cliCore_, [this, frame]() { return cliCore_->Send(frame); });
}