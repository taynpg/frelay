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

private:
    void onReadyRead();
    void onDisconnected();

private:
    void UseFrame(QSharedPointer<FrameBuffer> frame);
    bool Send(QSharedPointer<FrameBuffer> frame);
    bool Send(const char* data, qint64 len);

public:
    QMutex conMutex_;
    QString remoteID_;
    QTcpSocket* socket_;
    QByteArray recvBuffer_;

    std::function<void(const QString& path)> pathCall_;
};

#endif   // CLIENTCORE_H