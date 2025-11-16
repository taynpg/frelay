#include "FileInfoForm.h"

#include <QClipboard>
#include <Util.h>

#include "ui_FileInfoForm.h"

FileInfo::FileInfo(QWidget* parent) : QDialog(parent), ui(new Ui::FileInfo)
{
    ui->setupUi(this);
    InitControl();
    setFixedSize(minimumSizeHint());
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

    connect(ui->btnClose, &QPushButton::clicked, this, &FileInfo::close);
    connect(ui->btnCopyDirPath, &QPushButton::clicked, this, [this]() {
        QClipboard* clip = QApplication::clipboard();
        clip->setText(ui->pedDir->toPlainText());
    });
    connect(ui->btnCopyFileName, &QPushButton::clicked, this, [this]() {
        QClipboard* clip = QApplication::clipboard();
        clip->setText(ui->pedName->toPlainText());
    });
    connect(ui->btnCopyFull, &QPushButton::clicked, this, [this]() {
        auto d = ui->pedDir->toPlainText();
        auto f = ui->pedName->toPlainText();
        auto r = Util::Join(d, f);
        QClipboard* clip = QApplication::clipboard();
        clip->setText(r);
    });
}
