#include "Transform.h"

#include <QMessageBox>

#include "ui_Transform.h"

TransForm::TransForm(QWidget* parent) : QDialog(parent), ui(new Ui::TransForm)
{
    ui->setupUi(this);

    connect(this, &TransForm::sigProgress, this, &TransForm::setProgress);
    connect(this, &TransForm::sigDone, this, &TransForm::handleDone);
    connect(this, &TransForm::sigFailed, this, &TransForm::handleFailed);
    connect(this, &TransForm::sigSetUi, this, &TransForm::handleUI);
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
        sigSetUi(task);
        if (task.isUpload) {
            fileTrans_->ReqSendFile(task);
            while (true) {
                auto progress = fileTrans_->GetSendProgress();
                if (progress < 0) {
                    sigFailed();
                    break;
                }
                if (progress >= 100.0) {
                    sigDone();
                    break;
                }
                sigProgress(progress);
                QThread::msleep(10);
            }
        } else {
            fileTrans_->ReqDownFile(task);
            while (true) {
                auto progress = fileTrans_->GetDownProgress();
                if (progress < 0) {
                    sigFailed();
                    break;
                }
                if (progress >= 100.0) {
                    sigDone();
                    break;
                }
                sigProgress(progress);
                QThread::msleep(10);
            }
        }
    }
}

void TransForm::setProgress(double val)
{
    ui->progressBar->setValue(val);
}

void TransForm::handleFailed()
{
    ui->progressBar->setValue(0);
}

void TransForm::handleDone()
{
    ui->progressBar->setValue(0);
}

void TransForm::handleUI(const TransTask& task)
{
    if (task.isUpload) {
        ui->edFrom->setText(task.localId);
        ui->edTo->setText(task.remoteId);
        ui->pedFrom->setPlainText(task.localPath);
        ui->pedTo->setPlainText(task.remotePath);
    }
    else {
        ui->edFrom->setText(task.localId);
        ui->edTo->setText(task.remoteId);
        ui->pedFrom->setPlainText(task.remotePath);
        ui->pedTo->setPlainText(task.localPath);
    }
}

void TransForm::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    workTh_ = new TranFromTh(this, this);
    //fileTrans_->moveToThread(workTh_);
    connect(workTh_, &QThread::finished, fileTrans_, &QObject::deleteLater);
    workTh_->start();
}
