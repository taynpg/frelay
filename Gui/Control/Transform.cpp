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

void TransForm::SetTasks(const QVector<TransTask>& tasks)
{
    tasks_ = tasks;
}

void TransForm::StartExecTask()
{
    for (const auto& task : tasks_) {
        InfoMsg infoReq;
        if (task.isUpload) {
            if (!clientCore_->Send<InfoMsg>(infoReq, FBT_CLI_REQ_SEND, task.remoteId)) {
                QMessageBox::information(this, tr("Error"), tr("Send info request failed."));
                return;
            }
        }
        else {

        }
    }
}

void TransForm::StopExecTask()
{
}

void TransForm::SetClientCore(ClientCore* clientCore)
{
    clientCore_ = clientCore;
}
