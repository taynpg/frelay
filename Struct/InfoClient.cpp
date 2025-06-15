#include "InfoClient.h"

QDataStream& operator<<(QDataStream& data, const InfoClient& info)
{
    info.serialize(data);
    return data;
}

QDataStream& operator>>(QDataStream& data, InfoClient& info)
{
    info.deserialize(data);
    return data;
}
