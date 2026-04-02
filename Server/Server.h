// Server.h
#ifndef SERVER_H
#define SERVER_H

#include <FlowController.h>
#include <QMap>
#include <QReadWriteLock>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <memory>

#include "Protocol.h"

struct ShowData {
    qint64 bytesToWrite{};
    int curDelay{};
    std::shared_ptr<FlowController> fl;
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
    void sigByteToWrite(ShowData val);

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onReadyRead();
    void monitorClients();

private:
    QByteArray getClients();

private:
    struct ClientInfo {
        QTcpSocket* socket;
        QString id;
        qint64 connectTime;
        QByteArray buffer;
    };

    void processClientData(QSharedPointer<ClientInfo> client);
    bool forwardData(QSharedPointer<ClientInfo> client, QSharedPointer<FrameBuffer> frame);
    void replyRequest(QSharedPointer<ClientInfo> client, QSharedPointer<FrameBuffer> frame);
    bool sendData(QTcpSocket* socket, QSharedPointer<FrameBuffer> frame);

    // 流量控制
    bool sendWithFlowCheck(QTcpSocket* fsoc, QTcpSocket* tsoc, QSharedPointer<FrameBuffer> frame);
    BlockLevel getBlockLevel(QTcpSocket* socket);

    QString id_;
    QMap<QString, std::shared_ptr<ShowData>> curShow_;
    QMap<QString, QSharedPointer<ClientInfo>> clients_;
    QReadWriteLock rwLock_;
    QTimer* monitorTimer_;
};

#endif   // SERVER_H
