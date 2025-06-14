#ifndef CLIENTCORE_H
#define CLIENTCORE_H

#include <Protocol.h>
#include <QDataStream>
#include <QHostAddress>
#include <QMutex>
#include <QMutexLocker>
#include <QTcpSocket>
#include <QThread>

class ClientCore : public QObject
{
    Q_OBJECT

public:
    ClientCore(QObject* parent = nullptr);
    ~ClientCore();

public:
    bool Connect(const QString& ip, quint16 port);
    void Disconnect();

public:
    QTcpSocket* socket_;
    QMutex conMutex_;
    // QSharedPointer<ClientUserInterface> cf;

    std::function<void(const QString& path)> pathCall_;

    QString remoteID_;
    QByteArray recvBuffer_;
};

#endif   // CLIENTCORE_H