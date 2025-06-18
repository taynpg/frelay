#include "Transform.h"
#include <QMessageBox>

#include "ui_Transform.h"

TransForm::TransForm(QWidget* parent) : QDialog(parent), ui(new Ui::TransForm)
{
    ui->setupUi(this);
}

TransForm::~TransForm()
{
    delete ui;
}

void TransForm::SetClientCore(ClientCore* clientCore)
{
    clientCore_ = clientCore;
}
