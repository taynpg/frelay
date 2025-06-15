#include "frelayGUI.h"

#include <QSplitter>

#include "./ui_frelayGUI.h"

static LogPrint* logPrint = nullptr;

frelayGUI::frelayGUI(QWidget* parent) : QMainWindow(parent), ui(new Ui::frelayGUI)
{
    ui->setupUi(this);
    InitControl();
    ControlSignal();
    ControlLayout();
    resize(1500, 800);
}

frelayGUI::~frelayGUI()
{
    delete ui;
}

void frelayGUI::InitControl()
{
    logPrint = new LogPrint(this);
    clientCore_ = new ClientCore();

    connecter_ = new Connecter(this);
    connecter_->SetClientCore(clientCore_);
    connecter_->SetRemoteCall([this](const QString& id) { clientCore_->SetRemoteID(id); });

    localFile_ = new FileManager(this);
    remoteFile_ = new FileManager(this);
    localFile_->SetModeStr(tr("Local:"));
    remoteFile_->SetModeStr(tr("Remote:"), 1, clientCore_);

    tabWidget_ = new QTabWidget(this);
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

    sFile->addWidget(localFile_);
    sFile->addWidget(remoteFile_);

    splitter->addWidget(sTop);
    splitter->addWidget(sFile);
    setCentralWidget(splitter);
}

void frelayGUI::ControlMsgHander(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Q_UNUSED(context);

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
    case QtCriticalMsg:
    case QtFatalMsg:
        logPrint->Error(msg);
        break;
    default:
        logPrint->Error("Unknown QtMsgType type.");
        break;
    }
}

void frelayGUI::closeEvent(QCloseEvent* event)
{
    QMainWindow::closeEvent(event);
}
