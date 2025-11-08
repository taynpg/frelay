#ifndef INFO_MSG_H
#define INFO_MSG_H

#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QMap>
#include <QString>
#include <QVector>

struct PropertyData {
    QString uuid;
    QString command;
    QString userAction;
    QString localPath;
    QString remotePath;
    qint32 state;
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
        data << static_cast<qint32>(list.size());
        for (const auto& item : list) {
            data << item;
        }
        data << static_cast<qint32>(mapData.size());
        for (auto it = mapData.constBegin(); it != mapData.constEnd(); ++it) {
            data << it.key();
            data << it.value().uuid << it.value().command << it.value().userAction
                 << it.value().localPath << it.value().remotePath << it.value().state << it.value().properE;
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
        mapData.clear();
        for (int i = 0; i < mapSize; ++i) {
            QString key;
            PropertyData prop;
            data >> key;
            data >> prop.uuid >> prop.command >> prop.userAction >> prop.localPath
                >> prop.remotePath >> prop.state >> prop.properE;
            mapData.insert(key, prop);
        }
    }
};
QDataStream& operator<<(QDataStream& data, const InfoMsg& info);
QDataStream& operator>>(QDataStream& data, InfoMsg& info);

#endif   // INFO_MSG_H
