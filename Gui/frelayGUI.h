#ifndef FRELAYGUI_H
#define FRELAYGUI_H

#include <ClientCore.h>
#include <QFile>
#include <QDialog>
#include <QTabWidget>

#include "Control/CompareControl.h"
#include "Control/ConnectControl.h"
#include "Control/FileControl.h"
#include "Form/Transform.h"
#include "GuiUtil/Config.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class frelayGUI;
}
QT_END_NAMESPACE

class frelayGUI : public QDialog
{
    Q_OBJECT

public:
    frelayGUI(QWidget* parent = nullptr);
    ~frelayGUI();

private:
    void InitControl();
    void ControlSignal();
    void ControlLayout();

public:
    static void ControlMsgHander(QtMsgType type, const QMessageLogContext& context, const QString& msg);

public slots:
    void HandleTask(const QVector<TransTask>& tasks);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    Ui::frelayGUI* ui;
    QTabWidget* tabWidget_;
    Connecter* connecter_;
    FileManager* localFile_;
    FileManager* remoteFile_;
    ClientCore* clientCore_;
    TransForm* transform_;
    Compare* compare_;
    std::shared_ptr<FrelayConfig> config_;
};
#endif   // FRELAYGUI_H
