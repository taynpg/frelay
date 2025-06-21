#include <QApplication>
#include <QDir>
#include <QFile>
#include <SingleApplication>

#include "frelayGUI.h"

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

    CRASHELPER_MARK_ENTRY();
    SingleApplication a(argc, argv);

#ifdef _WIN32
    QFont font("Microsoft YaHei", 9);
    a.setFont(font);
    // a.setStyle("fusion");
    a.setStyle("windows");
#endif

    qInstallMessageHandler(frelayGUI::ControlMsgHander);
    qRegisterMetaType<QSharedPointer<FrameBuffer>>("QSharedPointer<FrameBuffer>");
    qRegisterMetaType<InfoClientVec>("InfoClientVec");
    qRegisterMetaType<DirFileInfoVec>("DirFileInfoVec");

    frelayGUI w;

    QObject::connect(&a, &SingleApplication::instanceStarted, &w, [&w]() {
        w.showNormal();
        w.raise();
        w.activateWindow();
    });

    w.show();
    return a.exec();
}
