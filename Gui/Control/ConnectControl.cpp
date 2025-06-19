#include "ConnectControl.h"

#include <InfoMsg.h>
#include <InfoPack.hpp>
#include <future>

#include "GuiUtil/Public.h"
#include "ui_ConnectControl.h"

Connecter::Connecter(QWidget* parent) : QWidget(parent), ui(new Ui::Connecter)
{
    ui->setupUi(this);
    InitControl();
}

Connecter::~Connecter()
{
    delete ui;
}

void Connecter::SetClientCore(ClientCore* clientCore)
{
    clientCore_ = clientCore;
    connect(clientCore_, &ClientCore::sigClients, this, &Connecter::HandleClients);
}

void Connecter::SetRemoteCall(const std::function<void(const QString& id)>& call)
{
    remoteCall_ = call;
}

void Connecter::HandleClients(const InfoClientVec& clients)
{
    model_->removeRows(0, ui->listView->model()->rowCount());
    for (const auto& client : clients.vec) {
        auto* item = new QStandardItem(client.id);
        model_->appendRow(item);
    }
}

void Connecter::Connect()
{
    auto ip = ui->edIP->text().trimmed();
    auto port = ui->edPort->text().trimmed();
    if (ip.isEmpty() || port.isEmpty()) {
        FTCommon::msg(this, tr("IP or Port is empty."));
        return;
    }

    sockWorker_ = new SocketWorker(clientCore_, nullptr);
    clientCore_->moveToThread(sockWorker_);

    connect(sockWorker_, &SocketWorker::conSuccess, this, [this]() {
        setState(ConnectState::CS_CONNECTED);
        qInfo() << QString(tr("Connected."));
    });

    connect(sockWorker_, &SocketWorker::conFailed, this, [this]() {
        setState(ConnectState::CS_DISCONNECT);
        qInfo() << QString(tr("Connect failed."));
    });

    connect(sockWorker_, &SocketWorker::connecting, this, [this]() {
        setState(ConnectState::CS_CONNECTING);
        qInfo() << QString(tr("Connecting..."));
    });

    connect(sockWorker_, &SocketWorker::disconnected, this, [this]() {
        setState(ConnectState::CS_DISCONNECT);
        qInfo() << QString(tr("Disconnected."));
    });

    connect(sockWorker_, &QThread::finished, sockWorker_, &QObject::deleteLater);
    sockWorker_->SetConnectInfo(ip, port.toInt());
    sockWorker_->start();
}

void Connecter::setState(ConnectState cs)
{
    switch (cs) {
    case CS_CONNECTING:
        ui->btnConnect->setEnabled(false);
        ui->btnDisconnect->setEnabled(false);
        break;
    case CS_CONNECTED:
        ui->btnConnect->setEnabled(false);
        ui->btnDisconnect->setEnabled(true);
        break;
    case CS_DISCONNECT:
        ui->btnConnect->setEnabled(true);
        ui->btnDisconnect->setEnabled(false);
        break;
    default:
        break;
    }
}

void Connecter::RefreshClient()
{
    InfoMsg info;
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->data = infoPack(info);
    frame->type = FBT_SER_MSG_ASKCLIENTS;
    auto sendRet = ClientCore::asyncInvoke(clientCore_, [this, frame]() { return clientCore_->Send(frame); });
    if (!sendRet) {
        qCritical() << QString(tr("send ask client list failed."));
        return;
    }
    qInfo() << QString(tr("ask client list..."));
}

void Connecter::ShowContextMenu(const QPoint& pos)
{
    auto index = ui->listView->indexAt(pos);

    if (!index.isValid()) {
        return;
    }
    menu_->exec(QCursor::pos());
}

std::string Connecter::getCurClient()
{
    return ui->elbClient->text().toStdString();
}

void Connecter::InitControl()
{
    ui->btnDisconnect->setEnabled(false);
    ui->edIP->setText("127.0.0.1");
    ui->edPort->setText("9009");
    connect(ui->btnConnect, &QPushButton::clicked, this, &Connecter::Connect);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &Connecter::RefreshClient);
    connect(this, &Connecter::sendConnect, this, &Connecter::setState);

    model_ = new QStandardItemModel(this);
    ui->listView->setModel(model_);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->listView, &QListView::customContextMenuRequested, this, &Connecter::ShowContextMenu);

    menu_ = new QMenu(this);
    QAction* acUseThis = menu_->addAction(tr("UseThis"));
    connect(acUseThis, &QAction::triggered, this, [this]() {
        auto index = ui->listView->currentIndex();
        if (!index.isValid()) {
            return;
        }
        auto* item = model_->itemFromIndex(index);
        if (!item) {
            return;
        }
        auto name = item->text();
        ui->elbClient->setText(name);
        ui->elbClient->setStyleSheet("color: blue;");
        remoteCall_(name);
    });

    setMaximumSize(300, 300);
}
