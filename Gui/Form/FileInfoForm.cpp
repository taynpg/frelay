#include "FileInfoForm.h"

#include "ui_FileInfoForm.h"

FileInfo::FileInfo(QWidget* parent) : QDialog(parent), ui(new Ui::FileInfo)
{
    ui->setupUi(this);
    InitControl();
    setWindowTitle(tr("属性"));
}

FileInfo::~FileInfo()
{
    delete ui;
}

int FileInfo::exec()
{
    ui->pedDir->setPlainText(dirName_);
    ui->pedName->setPlainText(baseName_);
    ui->lbSize->setText(fileSize_);
    ui->lbTime->setText(fileTime_);
    ui->lbType->setText(fileType_);
    return QDialog::exec();
}

void FileInfo::InitControl()
{
    ui->pedDir->setReadOnly(true);
    ui->pedName->setReadOnly(true);
}
