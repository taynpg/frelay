#include <QCoreApplication>
#include <Util.h>
#include <fversion.h>
#include <iostream>
#include <memory>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "Console.h"

int main(int argc, char* argv[])
{
    qRegisterMetaType<QSharedPointer<FrameBuffer>>("QSharedPointer<FrameBuffer>");
    qRegisterMetaType<InfoClientVec>("InfoClientVec");
    qRegisterMetaType<DirFileInfoVec>("DirFileInfoVec");
    qRegisterMetaType<TransTask>("TransTask");
    qRegisterMetaType<QVector<QString>>("QVector<QString>");

#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
#endif

    auto ver = Util::GetVersion();
    std::cout << "==============> " << ver.toStdString() << std::endl;

    if (argc < 3) {
        std::cerr << "==============> Usage arg is ip port." << std::endl;
        return 0;
    }

    QCoreApplication app(argc, argv);

    Util::InitLogger("frelayConsole.log", "frelayConsole");
    qInstallMessageHandler(Util::ConsoleMsgHander);

    auto core = std::make_shared<ClientCore>();
    auto helper = std::make_shared<ConsoleHelper>();

    helper->SetIpPort(argv[1], QString("%1").arg(argv[2]).toInt());
    helper->RunWorker(core.get());
    helper->Connect();

    QObject::connect(core.get(), &ClientCore::sigDisconnect, [&app](){
        qWarning() << "Abnormal state, quit...";
        app.exit();
    });

    return app.exec();
}
