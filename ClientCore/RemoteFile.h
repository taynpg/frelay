#ifndef REMOTE_FILE_H
#define REMOTE_FILE_H

#include <InfoMsg.h>
#include <Util.h>

#include "ClientCore.h"

class RemoteFile : public DirFileHelper
{
public:
    RemoteFile() = default;
    ~RemoteFile() override = default;

public:
    void setClientCore(ClientCore* cliCore);

public:
    bool GetHome() override;
    bool GetDirFile(const QString& dir) override;

private:
    ClientCore* cliCore_;
};

#endif   // REMOTE_FILE_H