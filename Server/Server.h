// Server.h
#ifndef SERVER_H
#define SERVER_H

#include <QMap>
#include <QReadWriteLock>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <memory>

#include "ClientThread.h"
#include "Protocol.h"

struct ClientInfo
{
    QSharedPointer<ClientThread> clientWorker{};
    QThread* workerTh{};
};

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(QObject* parent = nullptr);
    ~Server();

    bool startServer(quint16 port);
    void stopServer();

private slots:
    void onNewConnection();
    void onClientDisconnected(const QString& clientId);
    void onClientDataReceived(const QString& clientId, const QSharedPointer<FrameBuffer>& frame);
    void onClientHeartbeat(const QString& clientId);
    void monitorClients();

private:
    QByteArray getClients();
    bool forwardDataToClient(const QString& fromId, QSharedPointer<FrameBuffer> frame);

    QString id_;
    QMap<QString, ClientInfo> clients_;
    QReadWriteLock rwLock_;
    QTimer* monitorTimer_;

    static constexpr int NO_HEATBEAT_TIMEOUT = 10;
    static constexpr int MONITOR_HEART_SPED = 10 * 2;
};

#endif   // SERVER_H