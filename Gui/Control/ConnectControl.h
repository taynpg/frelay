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

class LogPrint;
class Connecter : public QWidget
{
    Q_OBJECT

public:
    explicit Connecter(QWidget* parent = nullptr);
    ~Connecter();

public:
    void SetClientCore(ClientCore* clientCore);
    void SetLogPrint(LogPrint* log);
    void SetRemoteCall(const std::function<void(const QString& id)>& call);
    void HandleClients(const InfoClientVec& clients);

signals:
    void sendConnect(ConnectState cs);

private:
    void InitControl();
    void Connect();
    void setState(ConnectState cs);
    void RefreshClient();
    void ShowContextMenu(const QPoint& pos);
    std::string getCurClient();

private:
    std::thread thConnect_;
    Ui::Connecter* ui;
    LogPrint* log_;
    bool thRun_{false};
    bool connceted_{false};
    std::thread thContext_;
    std::function<void(const QString& id)> remoteCall_;
    ClientCore* clientCore_;

private:
    QMenu* menu_;
    QStandardItemModel* model_;
};

#endif   // CONNECTCONTROL_H
