#include "Consumer.h"

#include <QThread>

Consumer::Consumer(QObject* parent)
{
}

void Consumer::SetDelay(int delay)
{
    consumeDelay = delay;
}

bool Consumer::Use(QByteArray data)
{
    if (consumeDelay > 0) {
        QThread::msleep(consumeDelay);
    }
    return true;
}