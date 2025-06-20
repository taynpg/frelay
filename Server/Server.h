// Server.h
#ifndef SERVER_H
#define SERVER_H

#include <QMap>
#include <QReadWriteLock>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <crashelper.h>

#include "Protocol.h"

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

    QString id_;
    QMap<QString, QSharedPointer<ClientInfo>> clients_;
    QReadWriteLock rwLock_;
    QTimer* monitorTimer_;
};

#endif   // SERVER_H