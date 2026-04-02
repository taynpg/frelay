#ifndef PRODUCER_H
#define PRODUCER_H

#include <QObject>
#include "flPub.hpp"

class Producer : public QObject
{
    Q_OBJECT
public:
    explicit Producer(QObject *parent = nullptr);

signals:
    void produce(QByteArray data);

public slots:
    void start(int size);
    void stop();
    void quit();
    void setDelay(int delay);

public:
    void produceTask();
    
private:
    int totlaSize_{};
    int curSize_{};
    int delay_{1};
    bool isStart_{};
    bool isRunning_{};
    int oneChuckSize_{ONE_CHUCK_SIZE};
};


#endif // PRODUCER_H