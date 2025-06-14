#ifndef INFO_PACK_HPP
#define INFO_PACK_HPP

#include <QByteArray>
#include <QDataStream>
#include <QDebug>

template <typename T> QByteArray infoPack(const T& obj)
{
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::ReadWrite);
    obj.serialize(stream);
    stream.device()->seek(0);
    return byteArray;
}

template <typename T> T infoUnpack(const QByteArray& byteArray)
{
    T obj;
    QDataStream stream(byteArray);
    obj.deserialize(stream);
    return obj;
}

#endif   // INFO_PACK_HPP