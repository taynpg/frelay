#ifndef LOCALFILE_H
#define LOCALFILE_H

#include <InfoDirFile.h>

#include "Util.h"

class LocalFile : public DirFileHelper
{
public:
    LocalFile(QObject* parent = nullptr);
    ~LocalFile() override = default;

public:
    bool GetHome() override;
    bool GetDirFile(const QString& dir) override;
    bool GetDirFile(const QString& dir, DirFileInfoVec& vec);
};

#endif   // LOCALFILE_H