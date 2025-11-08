#include "frelayGUI.h"

#include <Form/Loading.h>
#include <QLabel>
#include <QMessageBox>
#include <QScreen>
#include <QSplitter>
#include <QVBoxLayout>
#include <fversion.h>
#include "Control/Common.h"

#include "./ui_frelayGUI.h"
#include "Control/LogControl.h"

static LogPrint* logPrint = nullptr;

frelayGUI::frelayGUI(QWidget* parent) : QDialog(parent), ui(new Ui::frelayGUI)
{
    config_ = std::make_shared<FrelayConfig>();

    ui->setupUi(this);

    QString configRoot = Util::GetCurConfigPath("frelay");
    GlobalData::Ins()->SetConfigPath(configRoot.toStdString() + "/frelayConfig.json");

    InitControl();
    ControlSignal();
    ControlLayout();

    QRect availableGeometry = QGuiApplication::primaryScreen()->availableGeometry();
    int width = static_cast<int>(availableGeometry.width() * 0.8);
    int height = static_cast<int>(availableGeometry.height() * 0.6);
    resize(width, height);

    setWindowTitle(Util::GetVersion());
    setWindowFlags(windowFlags() | Qt::WindowMinMaxButtonsHint);

    // QLabel* permanent = new QLabel(this);
    // permanent->setFrameStyle(QFrame::Box | QFrame::Sunken);
    // permanent->setText(QString("%1 on %2").arg(VERSION_GIT_COMMIT, VERSION_GIT_BRANCH));
    // this->statusBar()->addPermanentWidget(permanent);
}

frelayGUI::~frelayGUI()
{
    delete ui;
}

void frelayGUI::InitControl()
{
    logPrint = new LogPrint(this);
    clientCore_ = new ClientCore();

    compare_ = new Compare(this);
    transform_ = new TransForm(this);
    transform_->SetClientCore(clientCore_);

    connecter_ = new Connecter(this);
    connecter_->RunWorker(clientCore_);
    connecter_->SetRemoteCall([this](const QString& id) { clientCore_->SetRemoteID(id); });
    connecter_->SetConfigPtr(config_);

    localFile_ = new FileManager(this);
    remoteFile_ = new FileManager(this);

    localFile_->SetModeStr(tr("本地："), 0, clientCore_);
    remoteFile_->SetModeStr(tr("远端："), 1, clientCore_);

    tabWidget_ = new QTabWidget(this);

    connect(localFile_, &FileManager::sigSendTasks, this, &frelayGUI::HandleTask);
    connect(remoteFile_, &FileManager::sigSendTasks, this, &frelayGUI::HandleTask);
    connect(compare_, &Compare::sigTasks, this, &frelayGUI::HandleTask);
    connect(compare_, &Compare::sigTryVisit, this, [this](bool local, const QString& path) {
        if (local) {
            localFile_->SetUiCurrentPath(path);
            localFile_->evtFile();
        } else {
            remoteFile_->SetUiCurrentPath(path);
            remoteFile_->evtFile();
        }
    });
    connect(connecter_, &Connecter::sigConfirmUse, remoteFile_, &FileManager::evtHome);
}

void frelayGUI::ControlSignal()
{
}

void frelayGUI::ControlLayout()
{
    auto* splitter = new QSplitter(Qt::Vertical);
    splitter->setHandleWidth(1);
    auto* sTop = new QSplitter(Qt::Horizontal);
    auto* sConnect = new QSplitter(Qt::Vertical);
    auto* sFile = new QSplitter(Qt::Horizontal);

    sTop->setHandleWidth(1);
    sConnect->setHandleWidth(1);
    sFile->setHandleWidth(1);

    sTop->addWidget(tabWidget_);
    sTop->addWidget(connecter_);
    tabWidget_->addTab(logPrint, tr("日志"));
    tabWidget_->addTab(compare_, tr("文件对照"));

    sFile->addWidget(localFile_);
    sFile->addWidget(remoteFile_);

    splitter->addWidget(sTop);
    splitter->addWidget(sFile);

    QList<int> sizes;
    sizes << height() * 2 / 5 << height() * 3 / 5;
    splitter->setSizes(sizes);

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget(splitter);
    setLayout(layout);
    // setCentralWidget(splitter);
}

void frelayGUI::ControlMsgHander(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    switch (type) {
    case QtDebugMsg:
        QMetaObject::invokeMethod(logPrint, "Debug", Qt::QueuedConnection, Q_ARG(QString, msg));
        break;
    case QtInfoMsg:
        QMetaObject::invokeMethod(logPrint, "Info", Qt::QueuedConnection, Q_ARG(QString, msg));
        break;
    case QtWarningMsg:
        QMetaObject::invokeMethod(logPrint, "Warn", Qt::QueuedConnection, Q_ARG(QString, msg));
        break;
    default:
        QMetaObject::invokeMethod(logPrint, "Error", Qt::QueuedConnection, Q_ARG(QString, msg));
        break;
    }
}

void frelayGUI::HandleTask(const QVector<TransTask>& tasks)
{
    if (!clientCore_->IsConnect()) {
        qCritical() << QString(tr("未连接到服务器。。。"));
        return;
    }

    // 检查文件
    CheckCondition cond(this);
    cond.SetTasks(tasks);
    cond.SetClientCore(clientCore_);

    LoadingDialog checking(this);
    checking.setTipsText("正在检查文件...");

    connect(&cond, &CheckCondition::sigCheckOver, &checking, &LoadingDialog::cancelBtnClicked);
    connect(&checking, &LoadingDialog::cancelWaiting, &cond, &CheckCondition::interrupCheck);
    connect(clientCore_, &ClientCore::sigMsgAnswer, &cond, &CheckCondition::recvFrame);

    cond.start();
    checking.exec();

    // 检查结果
    auto reTasks = cond.GetTasks();
    if (!CheckTaskResult(reTasks)) {
        return;
    }
    transform_->SetTasks(reTasks);
    transform_->exec();
}

bool frelayGUI::CheckTaskResult(QVector<TransTask>& tasks)
{
    bool isAccept = false;
    for (auto& task : tasks) {
        if (task.localCheckState == FCS_NORMAL && task.remoteCheckState == FCS_NORMAL) {
            continue;
        }
        if (task.isUpload) {
            if (task.localCheckState != FCS_NORMAL) {
                QMessageBox::information(this, tr("文件校验"), tr("本地文件校验失败，请检查文件是否存在：") + task.localPath);
                return false;
            }
            if (task.remoteCheckState != FCS_NORMAL && !isAccept) {
                auto msg = tr("远端不存在文件夹") + task.remotePath + "，需要自动创建吗？";
                auto ret =Common::GetAcceptThree(this, "操作确认", msg);
                if (ret == 0) {
                    continue;
                }
                else if (ret == 1) {
                    isAccept = true;
                    continue;
                }
                else {
                    return false;
                }
            }
        } else {
            if (task.localCheckState != FCS_NORMAL) {
                auto msg = tr("本地不存在文件夹") + task.localPath + "，需要自动创建吗？";
                auto ret =Common::GetAcceptThree(this, "操作确认", msg);
                if (ret == 0) {
                    continue;
                }
                else if (ret == 1) {
                    isAccept = true;
                    continue;
                }
                else {
                    return false;
                }
            }
            if (task.remoteCheckState != FCS_NORMAL) {
                QMessageBox::information(this, tr("文件校验"), tr("远端文件校验失败，请检查文件是否存在：") + task.remotePath);
                return false;
            }
        }
    }
    return true;
}

void frelayGUI::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
}
