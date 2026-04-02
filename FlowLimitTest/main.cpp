#include "FlowLimit.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FlowLimit w;
    w.show();
    return QCoreApplication::exec();
}
