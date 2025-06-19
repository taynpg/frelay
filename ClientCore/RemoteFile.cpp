﻿#include "RemoteFile.h"

#include <InfoPack.hpp>

#include "LocalFile.h"

RemoteFile::RemoteFile(QObject* parent) : DirFileHelper(parent)
{
}

void RemoteFile::setClientCore(ClientCore* cliCore)
{
    cliCore_ = cliCore;
    connect(cliCore_, &ClientCore::sigPath, this, [this](const QString& path) { sigHome(path); });
    connect(cliCore_, &ClientCore::sigFiles, this, [this](const DirFileInfoVec& files) { sigDirFile(files); });
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
    info.msg = dir;
    auto frame = cliCore_->GetBuffer(info, FBT_CLI_ASK_DIRFILE, cliCore_->GetRemoteID());
    return ClientCore::asyncInvoke(cliCore_, [this, frame]() { return cliCore_->Send(frame); });
}