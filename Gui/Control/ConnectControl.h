#ifndef CONNECTCONTROL_H
#define CONNECTCONTROL_H

#include <ClientCore.h>
#include <InfoClient.h>
#include <QMenu>
#include <QStandardItemModel>
#include <QWidget>

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
    void SetClientCore(ClientCore* clientCore);
    void SetRemoteCall(const std::function<void(const QString& id)>& call);
    void HandleClients(const InfoClientVec& clients);

signals:
    void sendConnect(ConnectState cs);

private:
    void InitControl();
    void Connect();
    void setState(ConnectState cs);
    void RefreshClient();
    void Disconnect();
    void ShowContextMenu(const QPoint& pos);
    std::string getCurClient();

private:
    Ui::Connecter* ui;
    bool connceted_{false};
    std::thread thContext_;
    std::function<void(const QString& id)> remoteCall_;
    ClientCore* clientCore_;

private:
    QMenu* menu_;
    SocketWorker* sockWorker_{};
    QStandardItemModel* model_;
};

#endif   // CONNECTCONTROL_H
