#ifndef CONSUMER_H
#define CONSUMER_H

#include <QObject>

#include "flPub.hpp"

class Consumer : public QObject
{
    Q_OBJECT
public:
    explicit Consumer(QObject* parent = nullptr);

public slots:
    bool Use(QByteArray data);
    void SetDelay(int delay);

private:
    int consumeDelay{100};
};

#endif   // CONSUMER_H