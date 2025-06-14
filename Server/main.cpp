#include <QCoreApplication>

#include "Server.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    Server server;
    if (!server.startServer(9009)) {
        return 1;
    }

    qDebug() << "TCP server is running. Press Ctrl+C to exit...";

    return app.exec();
}