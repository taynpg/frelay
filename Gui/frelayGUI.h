#ifndef FRELAYGUI_H
#define FRELAYGUI_H

#include <ClientCore.h>
#include <QFile>
#include <QMainWindow>
#include <QTabWidget>
#include <thread>

#include "Control/ConnectControl.h"
#include "Control/FileControl.h"
#include "Control/LogControl.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class frelayGUI;
}
QT_END_NAMESPACE

class frelayGUI : public QMainWindow
{
    Q_OBJECT

public:
    frelayGUI(QWidget* parent = nullptr);
    ~frelayGUI();

private:
    void InitControl();
    void ControlSignal();
    void ControlLayout();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    Ui::frelayGUI* ui;
    LogPrint* log_;
    QTabWidget* tabWidget_;
    Connecter* connecter_;
    FileManager* localFile_;
    FileManager* remoteFile_;
    ClientCore* clientCore_;
};
#endif   // FRELAYGUI_H
