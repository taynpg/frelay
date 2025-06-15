#include <QCoreApplication>

#include "Server.h"
#include <Util.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    Util::InitLogger("frelayServer.log", "frelayServer");
    qInstallMessageHandler(Util::ConsoleMsgHander);

    Server server;
    if (!server.startServer(9009)) {
        return 1;
    }

    qDebug() << "TCP server is running. Press Ctrl+C to exit...";

    return app.exec();
}