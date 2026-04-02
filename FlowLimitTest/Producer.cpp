#include "Producer.h"

#include <QThread>

Producer::Producer(QObject* parent)
{
}

void Producer::produceTask()
{
    isRunning_ = true;
    isStart_ = false;

    while (isRunning_) {
        if (!isStart_) {
            QThread::msleep(1);
            continue;
        }
        if (delay_ > 0) {
            QThread::msleep(delay_);
        }

        QByteArray data = QByteArray(ONE_CHUCK_SIZE, 'a');
        curSize_ += ONE_CHUCK_SIZE;
        emit produce(data);

        if (curSize_ >= totlaSize_) {
            curSize_ = 0;
            isStart_ = false;
        }
    }
}

void Producer::start(int size)
{
    totlaSize_ = size;
    isStart_ = true;
}

void Producer::stop()
{
    curSize_ = 0;
    isStart_ = false;
}

void Producer::quit()
{
    isRunning_ = false;
}

void Producer::setDelay(int delay)
{
    delay_ = delay;
}