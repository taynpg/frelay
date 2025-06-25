#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

void DateTimeTest()
{
    qDebug() << QDateTime::currentDateTime().toMSecsSinceEpoch();
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    qDebug() << "Running...";
    DateTimeTest();

    return app.exec();
}