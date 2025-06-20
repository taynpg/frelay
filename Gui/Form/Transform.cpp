#include "Transform.h"

#include <QMessageBox>

#include "ui_Transform.h"

TransForm::TransForm(QWidget* parent) : QDialog(parent), ui(new Ui::TransForm)
{
    ui->setupUi(this);

    setWindowTitle(tr("TransProgress"));
    ui->edFrom->setReadOnly(true);
    ui->edTo->setReadOnly(true);
    ui->pedFrom->setReadOnly(true);
    ui->pedTo->setReadOnly(true);
    ui->edTask->setReadOnly(true);

    connect(this, &TransForm::sigProgress, this, &TransForm::setProgress);
    connect(this, &TransForm::sigDone, this, &TransForm::handleDone);
    connect(this, &TransForm::sigFailed, this, &TransForm::handleFailed);
    connect(this, &TransForm::sigSetUi, this, &TransForm::handleUI);
    connect(this, &TransForm::sigTaskNum, this, &TransForm::showNum);
}

TransForm::~TransForm()
{
    exis_ = true;
    QThread::msleep(10);
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
    exis_ = false;
}

void TransForm::startTask()
{
    qInfo() << "TransForm::startTask enter....";
    curTaskNum_ = 0;
    for (auto& task : tasks_) {

        QString str = QString(tr("%1/%2")).arg(curTaskNum_).arg(tasks_.size());
        emit sigTaskNum(str);

        emit sigSetUi(task);
        if (task.isUpload) {
            fileTrans_->ReqSendFile(task);
            while (true) {
                if (exis_) {
                    return;
                }
                auto progress = fileTrans_->GetSendProgress();
                if (progress < 0) {
                    emit sigFailed();
                    break;
                }
                if (progress >= 100.0) {
                    emit sigDone();
                    break;
                }
                emit sigProgress(progress);
                QThread::msleep(10);
            }
        } else {
            fileTrans_->ReqDownFile(task);
            while (true) {
                if (exis_) {
                    return;
                }
                auto progress = fileTrans_->GetDownProgress();
                if (progress < 0) {
                    emit sigFailed();
                    break;
                }
                if (progress >= 100.0) {
                    emit sigDone();
                    break;
                }
                emit sigProgress(progress);
                QThread::msleep(10);
            }
        }
        ++curTaskNum_;

        str = QString(tr("%1/%2")).arg(curTaskNum_).arg(tasks_.size());
        emit sigTaskNum(str);
    }
    tasks_.clear();
    qInfo() << "TransForm::startTask exit....";
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
    ui->progressBar->setValue(100);
}

void TransForm::handleUI(const TransTask& task)
{
    if (task.isUpload) {
        ui->edFrom->setText(task.localId);
        ui->edTo->setText(task.remoteId);
        ui->pedFrom->setPlainText(task.localPath);
        ui->pedTo->setPlainText(task.remotePath);
    } else {
        ui->edFrom->setText(task.localId);
        ui->edTo->setText(task.remoteId);
        ui->pedFrom->setPlainText(task.remotePath);
        ui->pedTo->setPlainText(task.localPath);
    }
}

void TransForm::showNum(const QString& data)
{
    ui->edTask->setText(data);
}

void TransForm::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    workTh_ = new TranFromTh(this, this);
    // fileTrans_->moveToThread(workTh_);
    connect(workTh_, &QThread::finished, workTh_, &QObject::deleteLater);
    workTh_->start();
}

void TransForm::closeEvent(QCloseEvent* event)
{
    exis_ = true;
    QDialog::closeEvent(event);
}
