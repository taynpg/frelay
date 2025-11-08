#include "infoTest.h"

#include <QDebug>
#include <QElapsedTimer>
#include <InfoMsg.h>
#include <InfoPack.hpp>

int infoTest1()
{
    InfoClient ic;
    ic.id = "1234567890";
    ic.name = "Test";

    InfoClientVec vec;
    vec.vec.append(ic);

    QByteArray qa;
    QBuffer buf(&qa);
    buf.open(QIODevice::ReadWrite);

    QDataStream qs(&buf);
    vec.serialize(qs);
    buf.close();

    qDebug() << "Serialized data size:" << qa.size();
    qDebug() << "Serialized data:" << qa.toHex();

    QBuffer buf2(&qa);
    buf2.open(QIODevice::ReadWrite);
    QDataStream qs2(&buf2);
    InfoClientVec vec2;
    vec2.deserialize(qs2);
    buf2.close();

    return 0;
}

int infoTest2()
{
    InfoClient ic;
    ic.id = "1234567890";
    ic.name = "Test";

    InfoClientVec vec;
    vec.vec.append(ic);

    QByteArray qa;
    QDataStream qs(&qa, QIODevice::ReadWrite);
    vec.serialize(qs);

    qDebug() << "Serialized data size:" << qa.size();
    qDebug() << "Serialized data:" << qa.toHex();

    qs.device()->seek(0);
    InfoClientVec vec2;
    vec2.deserialize(qs);

    return 0;
}

void performanceTest()
{
    QByteArray data;
    QElapsedTimer timer;

    timer.start();
    QDataStream stream(&data, QIODevice::ReadWrite);
    for (int i = 0; i < 10000; ++i) {
        stream << i;
        stream.device()->seek(0);
        int val;
        stream >> val;
    }
    qDebug() << "Reuse stream:" << timer.elapsed() << "ms";

    timer.start();
    for (int i = 0; i < 10000; ++i) {
        QDataStream out(&data, QIODevice::WriteOnly);
        out << i;
        QDataStream in(&data, QIODevice::ReadOnly);
        int val;
        in >> val;
    }
    qDebug() << "New streams:" << timer.elapsed() << "ms";
}
int infoTest3()
{
    InfoMsg msg1;
    msg1.mapData["C++"].command = "exec";
    msg1.command = "FFASS0";

    auto bytes = infoPack<InfoMsg>(msg1);
    auto msg2 = infoUnpack<InfoMsg>(bytes);

    return 0;
}
