#ifndef FILECONTROL_H
#define FILECONTROL_H

#include <ClientCore.h>
#include <InfoDirFile.h>
#include <QWidget>
#include <Util.h>

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

private:
    void InitControl();
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
    std::shared_ptr<DirFileHelper> fileHelper_;
};

#endif   // FILECONTROL_H
