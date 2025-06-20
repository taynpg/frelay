#include <QApplication>
#include <QDir>
#include <QFile>

#include "frelayGUI.h"

#ifndef ZZXXCC
#include <crashelper.h>
#endif

int main(int argc, char* argv[])
{
    auto configDir = Util::GetCurConfigPath("frelay");
#ifdef _WIN32
    backward::SetDumpFileSavePath(configDir + "/dumpfile");
    backward::SetDumpLogSavePath(configDir + "/dumplog");
#else
    backward::SetDumpLogSavePath(configDir + QDir::separator() + "dumplog");
#endif

    CRASHELPER_MARK_ENTRY();
    QApplication a(argc, argv);

    qInstallMessageHandler(frelayGUI::ControlMsgHander);
    qRegisterMetaType<QSharedPointer<FrameBuffer>>("QSharedPointer<FrameBuffer>");
    qRegisterMetaType<InfoClientVec>("InfoClientVec");
    qRegisterMetaType<DirFileInfoVec>("DirFileInfoVec");

#ifdef _WIN32
    QFont font("Microsoft YaHei", 9);
    a.setFont(font);
    a.setWindowIcon(QIcon(":/ico/main.ico"));
    a.setStyle("Windows");
#endif

    frelayGUI w;
    w.show();
    return a.exec();
}
