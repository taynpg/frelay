#ifndef INFO_MSG_H
#define INFO_MSG_H

#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QString>
#include <QVector>

struct InfoMsg {
    qint32 mark{};
    QString msg;
    QString fromPath;
    QString toPath;
    quint64 size{};
    quint32 permissions{};

    void serialize(QDataStream& data) const
    {
        data << mark << msg << fromPath << toPath << size << permissions;
    }

    void deserialize(QDataStream& data)
    {
        data >> mark >> msg >> fromPath >> toPath >> size >> permissions;
    }
};

QDataStream& operator<<(QDataStream& data, const InfoMsg& info);
QDataStream& operator>>(QDataStream& data, InfoMsg& info);

#endif   // INFO_MSG_H