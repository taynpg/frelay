#ifndef INFO_MSG_H
#define INFO_MSG_H

#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QString>
#include <QVector>

struct InfoMsg {
    qint32 mark{};
    QString command;
    QString msg;
    QString fromPath;
    QString toPath;
    quint64 size{};
    quint32 permissions{};
    QVector<QString> list;

    void serialize(QDataStream& data) const
    {
        data << mark << command << msg << fromPath << toPath << size << permissions;
        data << list.size();
        for (const auto& item : list) {
            data << item;
        }
    }

    void deserialize(QDataStream& data)
    {
        data >> mark >> command >> msg >> fromPath >> toPath >> size >> permissions;
        qint32 listSize;
        data >> listSize;
        list.resize(listSize);
        for (auto& item : list) {
            data >> item;
        }
    }
};
QDataStream& operator<<(QDataStream& data, const InfoMsg& info);
QDataStream& operator>>(QDataStream& data, InfoMsg& info);

#endif   // INFO_MSG_H