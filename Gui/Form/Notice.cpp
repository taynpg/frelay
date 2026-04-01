#include "Notice.h"

#include "ui_Notice.h"

Notice::Notice(QWidget* parent) : QDialog(parent), ui(new Ui::Notice)
{
    BaseInit();
}

void Notice::BaseInit()
{
    ui->setupUi(this);

    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(350, 120);

    ui->plainTextEdit->setReadOnly(true);

    ui->plainTextEdit->setStyleSheet("QPlainTextEdit {"
                                     "  border: none;"
                                     "  background-color: transparent;"
                                     "  font-size: 13px;"
                                     "}");

    ui->btnExit->setStyleSheet("QPushButton {"
                               "  border: none;"
                               "  background-color: transparent;"
                               "  color: #666;"
                               "  font-size: 20px;"
                               "}"
                               "QPushButton:hover {"
                               "  color: #f00;"
                               "}");
    connect(ui->btnExit, &QPushButton::clicked, this, &Notice::btnExit);
}

Notice::Notice(QWidget* parent, const QString& message) : QDialog(parent), ui(new Ui::Notice)
{
    BaseInit();
    setWindowTitle("提示");
    ui->plainTextEdit->setPlainText(message);
}

Notice::Notice(QWidget* parent, const QString& title, const QString& message) : QDialog(parent), ui(new Ui::Notice)
{
    BaseInit();
    setWindowTitle(title);
    ui->plainTextEdit->setPlainText(message);
}

Notice::~Notice()
{
    delete ui;
}

void Notice::setTitle(const QString& title)
{
    setWindowTitle(title);
}

void Notice::setMessage(const QString& message)
{
    ui->plainTextEdit->setPlainText(message);
}

void Notice::btnExit()
{
    accept();   // 或使用 reject() 取决于需求
}