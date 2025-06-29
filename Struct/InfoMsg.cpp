#include "InfoMsg.h"

QDataStream& operator<<(QDataStream& data, const InfoMsg& info)
{
    info.serialize(data);
    return data;
}

QDataStream& operator>>(QDataStream& data, InfoMsg& info)
{
    info.deserialize(data);
    return data;
}
