#include <QApplication>
#include <QDir>
#include <QFile>
#include <SingleApplication>

#ifdef USE_FRELAY_THEME
#include <oclero/qlementine.hpp>
#endif

#include "frelayGUI.h"

int main(int argc, char* argv[])
{
    qRegisterMetaType<QSharedPointer<FrameBuffer>>("QSharedPointer<FrameBuffer>");
    qRegisterMetaType<InfoClientVec>("InfoClientVec");
    qRegisterMetaType<DirFileInfoVec>("DirFileInfoVec");
    qRegisterMetaType<TransTask>("TransTask");
    qRegisterMetaType<QVector<QString>>("QVector<QString>");

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    SingleApplication a(argc, argv);

#ifdef _WIN32
    QFont font("Microsoft YaHei", 9);
    a.setFont(font);
    // a.setStyle("fusion");
    // a.setStyle("windows");
#endif

    qInstallMessageHandler(frelayGUI::ControlMsgHander);

    frelayGUI w;

#ifdef USE_FRELAY_THEME
    auto* style = new oclero::qlementine::QlementineStyle(&a);
    QApplication::setStyle(style);
#endif

    QObject::connect(&a, &SingleApplication::instanceStarted, &w, [&w]() {
        w.showNormal();
        w.raise();
        w.activateWindow();
    });

    w.show();
    return a.exec();
}
