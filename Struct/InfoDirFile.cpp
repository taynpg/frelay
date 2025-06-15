#include "InfoDirFile.h"

QDataStream& operator<<(QDataStream& data, const DirFileInfo& info)
{
    info.serialize(data);
    return data;
}

QDataStream& operator>>(QDataStream& data, DirFileInfo& info)
{
    info.deserialize(data);
    return data;
}
