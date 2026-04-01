#ifndef INFO_MSG_H
#define INFO_MSG_H

#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QMap>
#include <QString>
#include <QVector>

struct FileStruct {
    qint32 state{};
    qint32 line{};
    QString path;
    QString root;
    QString mid;
    QString relative;
    QString mark;
};

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
    QString type;
    quint64 size{};
    quint32 permissions{};
    QVector<QString> listSend;
    QVector<QString> list;
    QMap<QString, PropertyData> mapData;
    FileStruct fst;
    QMap<QString, QVector<FileStruct>> infos;

    void serialize(QDataStream& data) const
    {
        data << mark << command << msg << fromPath << toPath << type << size << permissions;

        data << static_cast<qint32>(listSend.size());
        for (const auto& item : listSend) {
            data << item;
        }

        data << static_cast<qint32>(list.size());
        for (const auto& item : list) {
            data << item;
        }

        data << static_cast<qint32>(mapData.size());
        for (auto it = mapData.constBegin(); it != mapData.constEnd(); ++it) {
            data << it.key();
            data << it.value().uuid << it.value().command << it.value().userAction << it.value().localPath
                 << it.value().remotePath << it.value().state << it.value().properE;
        }
        data << fst.state << fst.line << fst.path << fst.root << fst.mid << fst.relative << fst.mark;
        data << static_cast<qint32>(infos.size());
        for (auto it = infos.constBegin(); it != infos.constEnd(); ++it) {
            data << it.key();
            data << static_cast<qint32>(it.value().size());
            for (const auto& fileStruct : it.value()) {
                data << fileStruct.state << fileStruct.line << fileStruct.path << fileStruct.root << fileStruct.mid
                     << fileStruct.relative << fileStruct.mark;
            }
        }
    }

    void deserialize(QDataStream& data)
    {
        data >> mark >> command >> msg >> fromPath >> toPath >> type >> size >> permissions;

        qint32 listSize;
        data >> listSize;
        listSend.resize(listSize);
        for (auto& item : listSend) {
            data >> item;
        }

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
            data >> prop.uuid >> prop.command >> prop.userAction >> prop.localPath >> prop.remotePath >> prop.state >>
                prop.properE;
            mapData.insert(key, prop);
        }

        data >> fst.state >> fst.line >> fst.path >> fst.root >> fst.mid >> fst.relative >> fst.mark;

        data >> mapSize;
        infos.clear();
        for (int i = 0; i < mapSize; ++i) {
            QString key;
            data >> key;

            qint32 vectorSize;
            data >> vectorSize;
            QVector<FileStruct> fileVector;
            fileVector.resize(vectorSize);
            for (int j = 0; j < vectorSize; ++j) {
                data >> fileVector[j].state >> fileVector[j].line >> fileVector[j].path >> fileVector[j].root >>
                    fileVector[j].mid >> fileVector[j].relative >> fileVector[j].mark;
            }
            infos.insert(key, fileVector);
        }
    }
};
QDataStream& operator<<(QDataStream& data, const InfoMsg& info);
QDataStream& operator>>(QDataStream& data, InfoMsg& info);

#endif   // INFO_MSG_H
