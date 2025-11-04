#include "ConnectControl.h"

#include <InfoMsg.h>
#include <InfoPack.hpp>
#include <QRegularExpression>
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
    heatBeat_ = new HeatBeat(clientCore_);
    clientCore_->moveToThread(sockWorker_);

    connect(clientCore_, &ClientCore::conSuccess, this, [this]() {
        setState(ConnectState::CS_CONNECTED);
        qInfo() << QString(tr("已连接。"));
    });

    connect(clientCore_, &ClientCore::sigYourId, this,
            [this](QSharedPointer<FrameBuffer> frame) { ui->edOwnID->setText(frame->data); });

    connect(clientCore_, &ClientCore::conFailed, this, [this]() {
        setState(ConnectState::CS_DISCONNECT);
        qInfo() << QString(tr("连接失败。"));
    });

    connect(clientCore_, &ClientCore::connecting, this, [this]() {
        setState(ConnectState::CS_CONNECTING);
        qInfo() << QString(tr("连接中......"));
    });

    connect(clientCore_, &ClientCore::sigDisconnect, this, [this]() {
        setState(ConnectState::CS_DISCONNECT);
        QStandardItemModel* model = qobject_cast<QStandardItemModel*>(ui->listView->model());
        if (model) {
            model->clear();
        }
        ui->elbClient->clear();
        clientCore_->SetRemoteID("");
        ui->edOwnID->setText("");
        qInfo() << QString(tr("已断开。"));
    });
    connect(clientCore_, &ClientCore::sigOffline, this, [this]() {
        ui->elbClient->clear();
        clientCore_->SetRemoteID("");
        RefreshClient();
    });

    connect(this, &Connecter::sigDoConnect, clientCore_, &ClientCore::DoConnect);
    connect(this, &Connecter::sigDisConnect, this,
            [this]() { QMetaObject::invokeMethod(clientCore_, "Disconnect", Qt::QueuedConnection); });
    connect(sockWorker_, &QThread::finished, sockWorker_, &QObject::deleteLater);

    heatBeat_->start();
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
        if (client.id == GlobalData::Ins()->GetLocalID()) {
            item->setForeground(QColor("red"));
        }
        model_->appendRow(item);
    }
}

void Connecter::SetConfigPtr(std::shared_ptr<FrelayConfig> config)
{
    config_ = config;
    auto ipPorts = config_->GetIpPort();
    for (const auto& ipPort : ipPorts) {
        ui->comboBox->addItem(ipPort);
    }
    if (ui->comboBox->count() > 0) {
        ui->comboBox->setCurrentIndex(0);
    }
}

void Connecter::Connect()
{
    QString ip;
    QString port;
    if (!parseIpPort(ui->comboBox->currentText(), ip, port)) {
        FTCommon::msg(this, QString(tr("IP或者端口不合法。")));
        return;
    }
    emit sigDoConnect(ip, port.toInt());
}

void Connecter::setState(ConnectState cs)
{
    config_->SaveIpPort(ui->comboBox->currentText());
    switch (cs) {
    case CS_CONNECTING:
        ui->btnConnect->setEnabled(false);
        ui->btnDisconnect->setEnabled(false);
        break;
    case CS_CONNECTED:
        ui->btnConnect->setEnabled(false);
        ui->btnDisconnect->setEnabled(true);
        RefreshClient();
        connect(heatBeat_, &HeatBeat::finished, heatBeat_, &QObject::deleteLater);
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
    qWarning() << QString(tr("断开连接。"));
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
        qCritical() << QString(tr("请求查询客户端列表失败。"));
        return;
    }
    qInfo() << QString(tr("刷新在线客户端列表。"));
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
    ui->edOwnID->setReadOnly(true);
    ui->label->setStyleSheet("color: blue;");
    ui->edOwnID->setStyleSheet("color: blue;");

    ui->btnDisconnect->setEnabled(false);
    ui->comboBox->setEditable(true);

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
        emit sigConfirmUse();
    });

    setMaximumWidth(300);
}

bool Connecter::parseIpPort(const QString& ipPort, QString& outIp, QString& outPort)
{
    QRegularExpression regex("^\\s*"
                             "("
                             "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                             "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                             "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\."
                             "(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
                             ")"
                             "\\s*:\\s*"
                             "("
                             "\\d{1,5}"
                             ")"
                             "\\s*$");

    QRegularExpressionMatch match = regex.match(ipPort);
    if (!match.hasMatch()) {
        return false;
    }

    outIp = match.captured(1);
    outPort = match.captured(2);

    bool portOk;
    int portNum = outPort.toInt(&portOk);
    return portOk && portNum > 0 && portNum <= 65535;
}
