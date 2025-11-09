#include "LocalFile.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

LocalFile::LocalFile(QObject* parent) : DirFileHelper(parent)
{
}

bool LocalFile::GetHome()
{
    auto home = Util::GetUserHome();
    emit sigHome(home, Util::GetLocalDrivers());
    return true;
}

bool LocalFile::GetDirFile(const QString& dir)
{
    DirFileInfoVec vec;
    if (!GetDirFile(dir, vec)) {
        return false;
    }
    emit sigDirFile(vec);
    return true;
}

bool LocalFile::GetDirFile(const QString& dir, DirFileInfoVec& vec)
{
    vec.vec.clear();
    vec.root = dir;

    QDir qdir(dir);
    if (!qdir.exists()) {
        err_ = tr("Path does not exist");
        return false;
    }

    if (!qdir.isReadable()) {
        err_ = tr("Directory is not readable");
        return false;
    }

    const auto entries = qdir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    for (const auto& entry : entries) {
        DirFileInfo info;
        info.fullPath = entry.absoluteFilePath();
        info.name = entry.fileName();
        info.permission = entry.permissions();

        if (entry.isDir()) {
            info.type = Dir;
            info.size = 0;
        } else if (entry.isFile()) {
            info.type = File;
            info.size = entry.size();
        } else {
            continue;
        }

        info.lastModified = entry.lastModified().toMSecsSinceEpoch();
        vec.vec.push_back(info);
    }

    return true;
}