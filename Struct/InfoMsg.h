#ifndef INFO_MSG_H
#define INFO_MSG_H

#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QString>
#include <QVector>
#include <QMap>

struct PropertyData {
    QString key;
    QString mark;
    QString properA;
    QString properB;
    QString properC;
    qint32 properD;
    qint32 properE;
};

struct InfoMsg {
    qint32 mark{};
    QString command;
    QString msg;
    QString fromPath;
    QString toPath;
    quint64 size{};
    quint32 permissions{};
    QVector<QString> list;
    QMap<QString, PropertyData> mapData;

    void serialize(QDataStream& data) const
    {
        data << mark << command << msg << fromPath << toPath << size << permissions;
        data << list.size();
        for (const auto& item : list) {
            data << item;
        }
        data << mapData.size();
        for (const auto& item : mapData) {
            data << item.key << item.mark << item.properA << item.properB << item.properC << item.properD << item.properE;
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
        qint32 mapSize;
        data >> mapSize;
        data >> mapSize;
        for (int i = 0; i < mapSize; ++i) {
            PropertyData prop;
            data >> prop.key >> prop.mark >> prop.properA >> prop.properB >> prop.properC >> prop.properD >> prop.properE;
            mapData.insert(prop.key, prop);
        }
    }
};
QDataStream& operator<<(QDataStream& data, const InfoMsg& info);
QDataStream& operator>>(QDataStream& data, InfoMsg& info);

#endif   // INFO_MSG_H