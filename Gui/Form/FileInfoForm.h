#ifndef FILEINFOFORM_H
#define FILEINFOFORM_H

#include <QDialog>

namespace Ui {
class FileInfo;
}

class FileInfo : public QDialog
{
    Q_OBJECT

public:
    explicit FileInfo(QWidget* parent = nullptr);
    ~FileInfo();

    int exec() override;

public:
    QString baseName_;
    QString dirName_;
    QString fileType_;
    QString fileTime_;
    QString fileSize_;

private:
    void InitControl();

private:
    Ui::FileInfo* ui;
};

#endif   // FILEINFOFORM_H
