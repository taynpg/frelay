#include "CompareControl.h"

#include "ui_CompareControl.h"

Compare::Compare(QWidget* parent) : QWidget(parent), ui(new Ui::Compare)
{
    ui->setupUi(this);
}

Compare::~Compare()
{
    delete ui;
}
