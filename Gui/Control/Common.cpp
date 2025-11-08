#include "Common.h"

#include <QMessageBox>

Common::Common() : QWidget(nullptr)
{
}

int Common::GetAcceptThree(QWidget* parent, const QString& notice, const QString& title)
{
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(notice);
    msgBox.setText(title);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::Cancel);
    int result = msgBox.exec();
    switch (result) {
    case QMessageBox::Yes:
        return 0;
    case QMessageBox::YesToAll:
        return 1;
    default:
        return -1;
    }
}
