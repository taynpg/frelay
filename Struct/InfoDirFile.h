#ifndef INFO_DIR_FILE_H
#define INFO_DIR_FILE_H

#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QString>
#include <QVector>

enum FileType : uint32_t { None = 0, File, Dir, Link, Other };

struct DirFileInfo {
    QString name;
    quint64 size{};
    FileType type = None;
    QString fullPath;
    quint16 permission{};
    quint64 lastModified{};

    void serialize(QDataStream& data) const
    {
        data << name << size << type << fullPath << permission << lastModified;
    }

    void deserialize(QDataStream& data)
    {
        data >> name >> size >> type >> fullPath >> permission >> lastModified;
    }
};

QDataStream& operator<<(QDataStream& data, const DirFileInfo& info);
QDataStream& operator>>(QDataStream& data, DirFileInfo& info);

struct DirFileInfoVec {
    QVector<DirFileInfo> vec;
    QString root;

    void serialize(QDataStream& data) const
    {
        data << vec.size();
        for (const auto& info : vec) {
            data << info;
        }
        data << root;
    }
    void deserialize(QDataStream& data)
    {
        qsizetype size;
        data >> size;
        vec.resize(size);
        for (quint32 i = 0; i < size; ++i) {
            data >> vec[i];
        }
        data >> root;
    }
};

#endif   // INFO_DIR_FILE_H