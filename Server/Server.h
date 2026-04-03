#ifndef SERVER_H
#define SERVER_H

#include <QMap>
#include <QReadWriteLock>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <memory>
#include <vector>

#include "ClientThread.h"
#include "Protocol.h"

struct ClientInfo {
    QSharedPointer<ClientThread> clientWorker{};
    QThread* workerTh{};
    qint64 lastHeartbeatCheck{0};
};

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(QObject* parent = nullptr);
    ~Server();

    bool startServer(quint16 port);
    void stopServer();

signals:
    void sigReqStop();

private slots:
    void onNewConnection();
    void onClientDisconnected(const QString& clientId);
    void onClientDataReceived(const QString& clientId, const QSharedPointer<FrameBuffer>& frame);
    void onClientHeartbeat(const QString& clientId);
    void monitorClients();
    void onClientHeartbeatTimeout(const QString& clientId, qint64 lastTime);

private:
    QByteArray getClients();
    bool forwardDataToClient(const QString& fromId, QSharedPointer<FrameBuffer> frame);

    // 线程安全的客户端查找
    QSharedPointer<ClientThread> findClientThread(const QString& clientId) const;

    // 清理已断开连接的客户端
    void cleanupDisconnectedClients();

    // 异步检查客户端活跃状态
    void checkClientAliveAsync(const QString& clientId);

    QString id_;
    QMap<QString, ClientInfo> clients_;
    mutable QReadWriteLock rwLock_;
    QTimer* monitorTimer_;

    // 待清理的客户端列表
    QVector<QString> clientsToRemove_;
    QMutex removeMutex_;

    static constexpr int NO_HEATBEAT_TIMEOUT = 10;
    static constexpr int MONITOR_HEART_SPED = 10 * 1000;   // 10秒检查一次
    static constexpr int MAX_CLIENTS = 100;
};

#endif   // SERVER_H