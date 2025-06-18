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
    fileTrans_ = new FileTrans(clientCore_);
}

void TransForm::SetTasks(const QVector<TransTask>& tasks)
{
    tasks_ = tasks;
}

void TransForm::startTask()
{
    for (auto& task : tasks_) {
        
    }
}

void TransForm::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    startTask();
}
