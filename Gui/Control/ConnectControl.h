#ifndef CONNECTCONTROL_H
#define CONNECTCONTROL_H

#include <ClientCore.h>
#include <InfoClient.h>
#include <QMenu>
#include <QStandardItemModel>
#include <QWidget>

#include "GuiUtil/Config.h"
#include "GuiUtil/Public.h"

namespace Ui {
class Connecter;
}

enum ConnectState {
    CS_DISCONNECT,
    CS_CONNECTING,
    CS_CONNECTED,
};

class Connecter : public QWidget
{
    Q_OBJECT

public:
    explicit Connecter(QWidget* parent = nullptr);
    ~Connecter();

public:
    void RunWorker(ClientCore* clientCore);
    void SetRemoteCall(const std::function<void(const QString& id)>& call);
    void HandleClients(const InfoClientVec& clients);
    void SetConfigPtr(std::shared_ptr<FrelayConfig> config);

signals:
    void sendConnect(ConnectState cs);
    void sigDoConnect(const QString& ip, quint16 port);
    void sigDisConnect();
    void sigConfirmUse();

private:
    void InitControl();
    void Connect();
    void setState(ConnectState cs);
    void RefreshClient();
    void Disconnect();
    void ShowContextMenu(const QPoint& pos);
    std::string getCurClient();
    bool parseIpPort(const QString& ipPort, QString& outIp, QString& outPort);

private:
    Ui::Connecter* ui;
    bool connceted_{false};
    std::function<void(const QString& id)> remoteCall_;
    ClientCore* clientCore_;

private:
    QMenu* menu_;
    SocketWorker* sockWorker_{};
    HeatBeat* heatBeat_{};
    QStandardItemModel* model_;
    std::shared_ptr<FrelayConfig> config_;
};

#endif   // CONNECTCONTROL_H
