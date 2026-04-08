#ifndef FRELAYGUI_H
#define FRELAYGUI_H

#include <ClientCore.h>
#include <QDialog>
#include <QFile>
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
    frelayGUI(QWidget* parent, std::shared_ptr<FrelayConfig> config);
    ~frelayGUI();

private:
    void InitControl();
    void ControlSignal();
    void ControlLayout();

public:
    static void ControlMsgHander(QtMsgType type, const QMessageLogContext& context, const QString& msg);

public slots:
    void HandleTask(const QVector<TransTask>& tasks);
    bool CheckTaskResult(QVector<TransTask>& tasks);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    Ui::frelayGUI* ui;
    Connecter* connecter_;
    FileManager* localFile_;
    FileManager* remoteFile_;
    ClientCore* clientCore_;
    TransForm* transform_;
    Compare* compare_;
    QTabWidget* tabWidget_;
    std::shared_ptr<FrelayConfig> config_;
};
#endif   // FRELAYGUI_H
