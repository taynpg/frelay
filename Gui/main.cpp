#include <QApplication>
#include <QFile>

#include "frelayGUI.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);

    qInstallMessageHandler(frelayGUI::ControlMsgHander);
    qRegisterMetaType<QSharedPointer<FrameBuffer>>("QSharedPointer<FrameBuffer>");
    qRegisterMetaType<InfoClientVec>("InfoClientVec");
    qRegisterMetaType<DirFileInfoVec>("DirFileInfoVec");

#ifdef _WIN32
    QFont font("Microsoft YaHei", 9);
    a.setFont(font);
    a.setWindowIcon(QIcon(":/ico/main.ico"));
    //a.setStyle("Windows");
#endif

    frelayGUI w;
    w.show();
    return a.exec();
}
