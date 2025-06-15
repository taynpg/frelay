#include "frelayGUI.h"

#include <QSplitter>

#include "./ui_frelayGUI.h"

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
    log_ = new LogPrint(this);

    clientCore_ = new ClientCore(this);

    connecter_ = new Connecter(this);
    connecter_->SetClientCore(clientCore_);
    connecter_->SetLogPrint(log_);
    connecter_->SetRemoteCall([this](const QString& id) { clientCore_->SetRemoteID(id); });

    localFile_ = new FileManager(this);
    remoteFile_ = new FileManager(this);
    localFile_->SetModeStr(tr("Local:"));
    remoteFile_->SetModeStr(tr("Remote:"), 1, clientCore_);
    localFile_->SetLogPrint(log_);
    remoteFile_->SetLogPrint(log_);

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
    tabWidget_->addTab(log_, tr("Log"));

    sFile->addWidget(localFile_);
    sFile->addWidget(remoteFile_);

    splitter->addWidget(sTop);
    splitter->addWidget(sFile);
    setCentralWidget(splitter);
}

void frelayGUI::closeEvent(QCloseEvent* event)
{
    QMainWindow::closeEvent(event);
}
