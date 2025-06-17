#include <QCoreApplication>

#include "Console.h"
#include <Util.h>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    Util::InitLogger("frelayConsole.log", "frelayConsole");
    qInstallMessageHandler(Util::ConsoleMsgHander);

    return app.exec();
}