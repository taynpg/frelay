#ifndef FILECONTROL_H
#define FILECONTROL_H

#include <ClientCore.h>
#include <FileTrans.h>
#include <InfoDirFile.h>
#include <QDialogButtonBox>
#include <QMenu>
#include <QMutex>
#include <QWidget>
#include <Util.h>
#include <map>
#include <memory>

namespace Ui {
class FileManager;
}

enum class SortMethod {
    SMD_BY_NAME_ASC,
    SMD_BY_NAME_DESC,
    SMD_BY_TIME_DESC,
    SMD_BY_TIME_ASC,
    SMD_BY_TYPE_DESC,
    SMD_BY_TYPE_ASC,
    SMD_BY_SIZE_DESC,
    SMD_BY_SIZE_ASC,
};

class FileManager : public QWidget
{
    Q_OBJECT

public:
    explicit FileManager(QWidget* parent = nullptr);
    ~FileManager();

public:
    QString GetRoot();
    void SetUiCurrentPath(const QString& path);
    void SetModeStr(const QString& modeStr, int type = 0, ClientCore* clientCore = nullptr);

signals:
    void sigSendTasks(const QVector<TransTask>& tasks);

private:
    void InitControl();
    void InitMenu();
    void ShowPath(const QString& path, const QVector<QString>& drivers);
    void ShowFile(const DirFileInfoVec& info);
    void doubleClick(int row, int column);
    void SetRoot(const QString& path);
    void SortFileInfo(SortMethod method);
    void HeaderClicked(int column);
    void FilterFile(const QVector<QString>& selectedTypes);
    void GenFilter();
    void ShowFilterForm();
    void CopyFullPath();
    void ShowProperties();
    void UpDown();
    
private:
    void OperNewFolder();
    void OperDelete();
    void OperRename();
    void WaitMsg();

public slots:
    void evtHome();
    void evtFile();
    void evtUp();
    void RefreshTab();

private:
    bool isRemote_;
    Ui::FileManager* ui;
    QMenu* menu_;
    ClientCore* cliCore_;
    QMutex cbMut_;
    QMutex tbMut_;
    QVector<QString> drivers_;
    QSet<QString> fileTypes_;
    QSet<QString> curSelectTypes_;
    DirFileInfoVec currentInfo_;
    DirFileInfoVec currentShowInfo_;
    std::map<int, SortMethod> sortMedRecord_;
    std::shared_ptr<DirFileHelper> fileHelper_;
};

#endif   // FILECONTROL_H
