#include <QCoreApplication>
#include <Util.h>
#include <iostream>

#include "Console.h"

#ifndef COMPILER_USE_MINGW
#include <crashelper.h>
#endif

int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cerr << "Usage arg is ip port." << std::endl;
        return 0;
    }

#ifndef COMPILER_USE_MINGW
    auto configDir = Util::GetCurConfigPath("frelay");
#ifdef _WIN32
    backward::SetDumpFileSavePath(configDir + "/dumpfile");
    backward::SetDumpLogSavePath(configDir + "/dumplog");
#else
    backward::SetDumpLogSavePath(configDir + "/dumplog");
#endif
#endif

    qRegisterMetaType<QSharedPointer<FrameBuffer>>("QSharedPointer<FrameBuffer>");
    qRegisterMetaType<InfoClientVec>("InfoClientVec");
    qRegisterMetaType<DirFileInfoVec>("DirFileInfoVec");
    qRegisterMetaType<TransTask>("TransTask");

    QCoreApplication app(argc, argv);

    Util::InitLogger("frelayConsole.log", "frelayConsole");
    qInstallMessageHandler(Util::ConsoleMsgHander);

    auto* core = new ClientCore();
    auto* helper = new ConsoleHelper();

    helper->SetIpPort(argv[1], QString("%1").arg(argv[2]).toInt());
    helper->RunWorker(core);
    helper->Connect();

    return app.exec();
}
