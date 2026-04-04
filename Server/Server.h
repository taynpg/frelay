#ifndef SERVER_H
#define SERVER_H

#include <QMap>
#include <QReadWriteLock>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

#include "ClientWorker.h"
#include "Protocol.h"

struct ClientInfo {
    QSharedPointer<ClientWorker> worker;
    QThread* workerTh;
    qint64 lastHeart;
};

enum DispatcherType {
    DPT_SEND_SUCCESS = 0,
    DPT_SEND_FAILED,
    DPT_NOT_FOUND
};

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(QObject* parent = nullptr);
    ~Server();

    bool startServer(quint16 port);
    void stopServer();

public slots:
    void handleClientFrame(const QString& id, QSharedPointer<FrameBuffer> frame);

private slots:
    void onNewConnection();

private:
    QByteArray getClients();
    DispatcherType dataDispatcher(const QString& id, QSharedPointer<FrameBuffer> frame);

private:
    QString id_;

    QMutex cliMut_;
    QReadWriteLock rwLock_;
    QMap<QString, QSharedPointer<ClientInfo>> clients_;

    static constexpr int NO_HEATBEAT_TIMEOUT = 10;
    static constexpr int MONITOR_HEART_SPED = 10 * 1000;   // 10秒检查一次
    static constexpr int MAX_CLIENTS = 100;
};

#endif