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

void Connecter::RunWorker(ClientCore* clientCore)
{
    clientCore_ = clientCore;
    connect(clientCore_, &ClientCore::sigClients, this, &Connecter::HandleClients);

    sockWorker_ = new SocketWorker(clientCore_, nullptr);
    clientCore_->moveToThread(sockWorker_);

    connect(clientCore_, &ClientCore::conSuccess, this, [this]() {
        setState(ConnectState::CS_CONNECTED);
        qInfo() << QString(tr("Connected."));
    });

    connect(clientCore_, &ClientCore::conFailed, this, [this]() {
        setState(ConnectState::CS_DISCONNECT);
        qInfo() << QString(tr("Connect failed."));
    });

    connect(clientCore_, &ClientCore::connecting, this, [this]() {
        setState(ConnectState::CS_CONNECTING);
        qInfo() << QString(tr("Connecting..."));
    });

    connect(clientCore_, &ClientCore::sigDisconnect, this, [this]() {
        setState(ConnectState::CS_DISCONNECT);
        qInfo() << QString(tr("Disconnected."));
    });

    connect(this, &Connecter::sigDoConnect, clientCore_, &ClientCore::DoConnect);
    connect(this, &Connecter::sigDisConnect, clientCore_, &ClientCore::Disconnect);
    connect(sockWorker_, &QThread::finished, sockWorker_, &QObject::deleteLater);

    sockWorker_->start();
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
    emit sigDoConnect(ip, port.toInt());
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
        RefreshClient();
        break;
    case CS_DISCONNECT:
        ui->btnConnect->setEnabled(true);
        ui->btnDisconnect->setEnabled(false);
        break;
    default:
        break;
    }
}

void Connecter::Disconnect()
{
    qWarning() << QString(tr("Disconnected requeset..."));
    emit sigDisConnect();
}

void Connecter::RefreshClient()
{
    InfoMsg info;
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->data = infoPack(info);
    frame->type = FBT_SER_MSG_ASKCLIENTS;
    auto sendRet = ClientCore::syncInvoke(clientCore_, frame);
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
    ui->edPort->setFixedWidth(70);

    connect(ui->btnConnect, &QPushButton::clicked, this, &Connecter::Connect);
    connect(ui->btnRefresh, &QPushButton::clicked, this, &Connecter::RefreshClient);
    connect(this, &Connecter::sendConnect, this, &Connecter::setState);

    model_ = new QStandardItemModel(this);
    ui->listView->setModel(model_);
    ui->listView->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->listView, &QListView::customContextMenuRequested, this, &Connecter::ShowContextMenu);
    connect(ui->btnDisconnect, &QPushButton::clicked, this, &Connecter::Disconnect);

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
        ui->elbClient->setStyleSheet("color: green;");
        remoteCall_(name);
    });

    setMaximumWidth(300);
}
