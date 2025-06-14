#include "ClientCore.h"

#include <QDebug>

ClientCore::ClientCore(QObject* parent) : QObject(parent)
{
}

ClientCore::~ClientCore()
{
}

bool ClientCore::Connect(const QString& ip, quint16 port)
{
    QMutexLocker locker(&conMutex_);
    if (!locker.isLocked()) {
        qWarning() << QString(tr("%1:%2 already connecting...")).arg(ip).arg(port);
        return false;
    }
    socket_->connectToHost(ip, port);
    if (!socket_->waitForConnected(3000)) {
        qCritical() << QString(tr("%1:%2 connect failed...")).arg(ip).arg(port);
        return false;
    }
    return true;
}

void ClientCore::Disconnect()
{
}
