#include "frelayGUI.h"

#include <QLabel>
#include <QScreen>
#include <QSplitter>
#include <QVBoxLayout>
#include <fversion.h>

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
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint);

    // QLabel* permanent = new QLabel(this);
    // permanent->setFrameStyle(QFrame::Box | QFrame::Sunken);
    // permanent->setText(QString("%1 on %2").arg(VERSION_GIT_COMMIT, VERSION_GIT_BRANCH));
    //this->statusBar()->addPermanentWidget(permanent);
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

    localFile_->SetModeStr(tr("Local:"), 0, clientCore_);
    remoteFile_->SetModeStr(tr("Remote:"), 1, clientCore_);

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
    tabWidget_->addTab(logPrint, tr("Log"));
    tabWidget_->addTab(compare_, tr("Compare"));

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
        qCritical() << QString(tr("Not connect to server."));
        return;
    }
    transform_->SetTasks(tasks);
    transform_->exec();
}

void frelayGUI::closeEvent(QCloseEvent* event)
{
    QDialog::closeEvent(event);
}
