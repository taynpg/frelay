#include <QCoreApplication>
#include <Util.h>

#include "Console.h"

#ifndef COMPILER_USE_MINGW
#include <crashelper.h>
#endif

int main(int argc, char* argv[])
{

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

    Util::InitLogger("frelayConsole.log", "frelayConsole");
    qInstallMessageHandler(Util::ConsoleMsgHander);

    return app.exec();
}