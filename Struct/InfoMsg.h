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

    void serialize(QDataStream& data) const
    {
        data << mark << msg;
    }

    void deserialize(QDataStream& data)
    {
        data >> mark >> msg;
    }
};

QDataStream& operator<<(QDataStream& data, const InfoMsg& info);
QDataStream& operator>>(QDataStream& data, InfoMsg& info);

#endif  // INFO_MSG_H