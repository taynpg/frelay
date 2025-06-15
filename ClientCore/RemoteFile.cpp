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
    return cliCore_->Send<InfoMsg>(info, FBT_CLI_ASK_HOME, cliCore_->GetRemoteID());
}

bool RemoteFile::GetDirFile(const QString& dir)
{
    InfoMsg info;
    info.msg = dir;
    return cliCore_->Send<InfoMsg>(info, FBT_CLI_ASK_DIRFILE, cliCore_->GetRemoteID());
}