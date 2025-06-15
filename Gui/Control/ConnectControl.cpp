#include "ConnectControl.h"

#include <InfoMsg.h>
#include <InfoPack.hpp>
#include <future>

#include "Control/LogControl.h"
#include "GuiUtil/Public.h"
#include "ui_ConnectControl.h"

Connecter::Connecter(QWidget* parent) : QWidget(parent), ui(new Ui::Connecter)
{
    ui->setupUi(this);
    InitControl();
}

Connecter::~Connecter()
{
    if (thConnect_.joinable()) {
        thConnect_.join();
    }
    delete ui;
}

void Connecter::SetClientCore(ClientCore* clientCore)
{
    clientCore_ = clientCore;
    clientCore_->SetClientsCall([this](const InfoClientVec& clients) { HandleClients(clients); });
}

void Connecter::SetLogPrint(LogPrint* log)
{
    log_ = log;
}

void Connecter::SetRemoteCall(const std::function<void(const QString& id)>& call)
{
    remoteCall_ = call;
}

void Connecter::HandleClients(const InfoClientVec& clients)
{
    model_->removeRows(0, ui->listView->model()->rowCount());
    for (const auto& client : clients.vec) {
        auto* item = new QStandardItem(client.name);
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
    auto task = [this, ip, port]() {
        emit sendConnect(ConnectState::CS_CONNECTING);
        connceted_ = clientCore_->Connect(ip, port.toInt());
        if (connceted_) {
            emit sendConnect(ConnectState::CS_CONNECTED);
        } else {
            emit sendConnect(ConnectState::CS_DISCONNECT);
        }
    };
    if (thConnect_.joinable()) {
        thConnect_.join();
    }
    thConnect_ = std::thread(task);
}

void Connecter::setState(ConnectState cs)
{
    switch (cs) {
    case CS_CONNECTING:
        ui->btnConnect->setEnabled(false);
        ui->btnDisconnect->setEnabled(false);
        log_->Info(tr("Connecting..."));
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
    if (!clientCore_->Send(frame)) {
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
