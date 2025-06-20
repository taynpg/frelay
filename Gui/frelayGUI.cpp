#include "frelayGUI.h"

#include <QLabel>
#include <QSplitter>
#include <fversion.h>

#include "./ui_frelayGUI.h"
#include "Control/LogControl.h"

static LogPrint* logPrint = nullptr;

frelayGUI::frelayGUI(QWidget* parent) : QMainWindow(parent), ui(new Ui::frelayGUI)
{
    ui->setupUi(this);
    InitControl();
    ControlSignal();
    ControlLayout();
    resize(1500, 800);

    setWindowTitle(QString(tr("frelay %1")).arg(VERSION_NUM));

    QLabel* permanent = new QLabel(this);
    permanent->setFrameStyle(QFrame::Box | QFrame::Sunken);
    permanent->setText(QString("%1 on %2").arg(VERSION_GIT_COMMIT, VERSION_GIT_BRANCH));
    this->statusBar()->addPermanentWidget(permanent);
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

    localFile_ = new FileManager(this);
    remoteFile_ = new FileManager(this);

    localFile_->SetModeStr(tr("Local:"), 0, clientCore_);
    localFile_->SetOtherSideCall([this]() { return remoteFile_->GetCurRoot(); });
    remoteFile_->SetModeStr(tr("Remote:"), 1, clientCore_);
    remoteFile_->SetOtherSideCall([this]() { return localFile_->GetCurRoot(); });

    tabWidget_ = new QTabWidget(this);

    connect(localFile_, &FileManager::sigSendTasks, this, &frelayGUI::HandleTask);
    connect(remoteFile_, &FileManager::sigSendTasks, this, &frelayGUI::HandleTask);
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
    tabWidget_->addTab(logPrint, tr("Log"));
    tabWidget_->addTab(compare_, tr("Compare"));

    sFile->addWidget(localFile_);
    sFile->addWidget(remoteFile_);

    splitter->addWidget(sTop);
    splitter->addWidget(sFile);
    setCentralWidget(splitter);
}

void frelayGUI::ControlMsgHander(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QMetaObject::invokeMethod(
        qApp,
        [type, msg]() {
            switch (type) {
            case QtDebugMsg:
                logPrint->Debug(msg);
                break;
            case QtInfoMsg:
                logPrint->Info(msg);
                break;
            case QtWarningMsg:
                logPrint->Warn(msg);
                break;
            default:
                logPrint->Error(msg);
                break;
            }
        },
        Qt::QueuedConnection);
}

void frelayGUI::HandleTask(const QVector<TransTask>& tasks)
{
    transform_->SetTasks(tasks);
    transform_->exec();
}

void frelayGUI::closeEvent(QCloseEvent* event)
{
    QMainWindow::closeEvent(event);
}
