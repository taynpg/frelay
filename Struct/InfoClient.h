#ifndef INFO_CLIENT_H
#define INFO_CLIENT_H

#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QString>
#include <QVector>

struct InfoClient {
    QString id;
    QString name;

    void serialize(QDataStream& data) const
    {
        data << id << name;
    }

    void deserialize(QDataStream& data)
    {
        data >> id >> name;
    }
};

QDataStream& operator<<(QDataStream& data, const InfoClient& info);
QDataStream& operator>>(QDataStream& data, InfoClient& info);

struct InfoClientVec {
    QVector<InfoClient> vec;

    void serialize(QDataStream& data) const
    {
        data << vec.size();
        for (const auto& info : vec) {
            data << info;
        }
    }
    void deserialize(QDataStream& data)
    {
        qsizetype size;
        data >> size;
        vec.resize(size);
        for (quint32 i = 0; i < size; ++i) {
            data >> vec[i];
        }
    }
};

#endif   // INFO_CLIENT_H