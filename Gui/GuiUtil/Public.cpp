#include "Public.h"

#include <QFileDialog>
#include <QMessageBox>

void FTCommon::msg(QWidget* parent, const QString& content)
{
    QMessageBox::information(parent, QObject::tr("提示"), content);
}

bool FTCommon::affirm(QWidget* parent, const QString& titile, const QString& content)
{
    QMessageBox questionBox(parent);
    questionBox.setText(content);
    questionBox.setWindowTitle(titile);
    questionBox.setIcon(QMessageBox::Question);
    questionBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    int result = questionBox.exec();
    if (result != QMessageBox::Yes) {
        return false;
    } else {
        return true;
    }
}

QString FTCommon::select_file(QWidget* parent, const QString& info, const QString& filter)
{
    QString filePath = QFileDialog::getOpenFileName(parent, info, QDir::homePath(), filter);
    return filePath;
}

QString FTCommon::GetAppPath()
{
    QString home = QDir::homePath();
    QDir dir(home);
    auto ret_path = dir.absoluteFilePath(".config/frelayGUI");
    QDir d(ret_path);
    if (!d.exists()) {
        d.mkpath(ret_path);
    }
    return ret_path;
}
