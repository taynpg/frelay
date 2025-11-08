#include "Transform.h"

#include <QFileInfo>
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
                QThread::msleep(2);
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
                QThread::msleep(2);
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
        ui->edFrom->setText(task.remoteId);
        ui->edTo->setText(task.localId);
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

CheckCondition::CheckCondition(QObject* parent) : QThread(parent)
{
}

void CheckCondition::SetClientCore(ClientCore* clientCore)
{
    clientCore_ = clientCore;
}

void CheckCondition::SetTasks(const QVector<TransTask>& tasks)
{
    tasks_ = tasks;
}

QVector<TransTask> CheckCondition::GetTasks() const
{
    return tasks_;
}

void CheckCondition::recvFrame(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    if (info.command == STRMSG_AC_ANSWER_FILE_EXIST) {

        for (auto& item : info.mapData) {
            auto it =
                std::find_if(tasks_.begin(), tasks_.end(), [&item](const TransTask& task) { return task.taskUUID == item.uuid; });
            if (it == tasks_.end()) {
                continue;
            }
            it->remoteCheckState = static_cast<FileCheckState>(item.state);
        }

        qInfo() << tr("检查结束......");
        msg_ = info.command;
        return;
    }
    msg_ = tr("收到未知信息，认为判断失败：") + info.command;
    qInfo() << msg_;
}

void CheckCondition::interrupCheck()
{
    if (!isAlreadyInter_) {
        isAlreadyInter_ = true;
        qWarning() << tr("中断文件校验......");
        emit sigCheckOver();
    }
}

void CheckCondition::run()
{
    qInfo() << tr("开始文件校验......");
    isRun_ = true;
    msg_.clear();
    isAlreadyInter_ = false;

    // 先检查本地文件是否存在
    for (auto& task : tasks_) {
        if (task.isUpload && !Util::FileExist(task.localPath)) {
            task.localCheckState = FCS_FILE_NOT_EXIST;
        }
        if (!task.isUpload && !Util::DirExist(task.localPath, true)) {
            task.localCheckState = FCS_DIR_NOT_EXIST;
        }
    }

    // 再检查远程文件是否存在
    InfoMsg msg;
    msg.command = STRMSG_AC_CHECK_FILE_EXIST;

    for (auto& task : tasks_) {
        msg.mapData[task.taskUUID].uuid = task.taskUUID;
        msg.mapData[task.taskUUID].command = task.isUpload ? STRMSG_AC_UP : STRMSG_AC_DOWN;
        msg.mapData[task.taskUUID].path = task.remotePath;
    }

    auto f = clientCore_->GetBuffer(msg, FBT_MSGINFO_ASK, clientCore_->GetRemoteID());
    if (!ClientCore::syncInvoke(clientCore_, f)) {
        auto errMsg = tr("检查远程文件存在性数据发送失败。");
        emit sigCheckOver();
        qCritical() << errMsg;
        return;
    }
    while (isRun_) {
        QThread::msleep(10);
        if (msg_.isEmpty()) {
            continue;
        }
        break;
    }
    isAlreadyInter_ = true;
    emit sigCheckOver();
    qInfo() << tr("文件校验结束......");
}
