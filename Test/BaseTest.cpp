#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <Util.h>
#include <QZip.h>

void DateTimeTest()
{
    qDebug() << QDateTime::currentDateTime().toMSecsSinceEpoch();
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    qDebug() << "Running...";
    DateTimeTest();

    QVector<QString> files;
    DirFileHelper::GetAllFiles("D:/备份软件", files);

    QZip zip;

    QStringList subDirs;
    QStringList subFiles;
    subDirs << "test";
    subFiles << "xmake.lua";
    zip.compress("D:/ceshi.zip", "D:\\Code\\zoost", subDirs, subFiles);
    zip.extractAll("D:/ceshi.zip", "D:/666");

    return app.exec();
}