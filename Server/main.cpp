#include <QCoreApplication>
#include <QDir>
#include <Util.h>

#include "Server.h"

#ifndef COMPILER_USE_MINGW
#include <crashelper.h>
#endif

int main(int argc, char* argv[])
{
    int port = 9009;
    if (argc < 2) {
        qDebug() << "Usage: frelayServer port.";
    } else {
        port = atoi(argv[1]);
    }

#ifndef COMPILER_USE_MINGW
    auto configDir = Util::GetCurConfigPath("frelay");
#ifdef _WIN32
    backward::SetDumpFileSavePath(configDir + "/dumpfile");
    backward::SetDumpLogSavePath(configDir + "/dumplog");
#else
    backward::SetDumpLogSavePath(configDir + QDir::separator() + "dumplog");
#endif
#endif

    QCoreApplication app(argc, argv);

    Util::InitLogger("frelayServer.log", "frelayServer");
    qInstallMessageHandler(Util::ConsoleMsgHander);

    Server server;
    if (!server.startServer(port)) {
        return 1;
    }

    qDebug() << "TCP server is running. Press Ctrl+C to exit...";

    return app.exec();
}