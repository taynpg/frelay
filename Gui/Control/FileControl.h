#ifndef FILECONTROL_H
#define FILECONTROL_H

#include <ClientCore.h>
#include <FileTrans.h>
#include <InfoDirFile.h>
#include <QWidget>
#include <Util.h>
#include <QMutex>
#include <QMenu>

namespace Ui {
class FileManager;
}

class FileManager : public QWidget
{
    Q_OBJECT

public:
    explicit FileManager(QWidget* parent = nullptr);
    ~FileManager();

public:
    void SetModeStr(const QString& modeStr, int type = 0, ClientCore* clientCore = nullptr);
    void SetOtherSideCall(const std::function<QString()>& call);
    QString GetCurRoot();

signals:
    void sigSendTasks(const QVector<TransTask>& tasks);

private:
    void InitControl();
    void InitMenu(bool remote = false);
    void ShowPath(const QString& path);
    void ShowFile(const DirFileInfoVec& info);
    void doubleClick(int row, int column);

private:
    void evtHome();
    void evtFile();
    void evtUp();

private:
    Ui::FileManager* ui;
    QString curRoot_;
    QMenu* menu_;
    ClientCore* cliCore_;
    QMutex cbMut_;
    QMutex tbMut_;
    std::shared_ptr<DirFileHelper> fileHelper_;
};

#endif   // FILECONTROL_H
