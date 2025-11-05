#include <QCoreApplication>
#include <QDir>
#include <Util.h>
#include <fversion.h>
#include <iostream>

#include "Server.h"

int main(int argc, char* argv[])
{
    auto ver = Util::GetVersion();
    std::cout << "==============> " << ver.toStdString() << std::endl;

    int port = 9009;
    if (argc < 2) {
        qInfo() << "==============> Usage: frelayServer port.";
    } else {
        port = atoi(argv[1]);
    }

    QCoreApplication app(argc, argv);

    Util::InitLogger("frelayServer.log", "frelayServer");
    qInstallMessageHandler(Util::ConsoleMsgHander);

    Server server;
    if (!server.startServer(port)) {
        return 1;
    }

    qInfo() << "TCP server is running. Press Ctrl+C to exit...";

    return app.exec();
}