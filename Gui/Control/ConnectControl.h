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
    QThread* th_;
    QThread* mainTh_;
    QStandardItemModel* model_;
};

class ConnectWorker : public QObject
{
    Q_OBJECT
public:
    explicit ConnectWorker(ClientCore* clientCore, QObject* parent = nullptr)
        : QObject(parent), clientCore_(clientCore)
    {
    }

public slots:
    void doConnect(const QString& ip, int port, QThread* parentThread)
    {
        emit connecting();
        bool connected = clientCore_->Connect(ip, port);
        clientCore_->moveToThread(parentThread);
        emit connectResult(connected);
    }

signals:
    void connectResult(bool success);
    void connecting();

private:
    ClientCore* clientCore_;
};

#endif   // CONNECTCONTROL_H
