#include <QCoreApplication>
#include <Util.h>
#include <iostream>
#include <fversion.h>

#include "Console.h"

int main(int argc, char* argv[])
{
    auto ver = Util::GetVersion();
    std::cout << "==============> " << ver.toStdString() << std::endl;

    if (argc < 3) {
        std::cerr << "==============> Usage arg is ip port." << std::endl;
        return 0;
    }

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
