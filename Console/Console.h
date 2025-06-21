#ifndef CONSOLE_H
#define CONSOLE_H

#include <ClientCore.h>

class ConsoleHelper : public QObject
{
    Q_OBJECT

public:
    ConsoleHelper(QObject* parent = nullptr);
    ~ConsoleHelper() override;

public:
    void RunWorker(ClientCore* clientCore);
    void SetIpPort(const QString& ip, quint16 port);
    void Connect();

signals:
    void sigDoConnect(const QString& ip, quint16 port);

private:
    QString ip_;
    quint16 port_{};
    SocketWorker* sockWorker_{};
    ClientCore* clientCore_;
};

#endif   // CONSOLE_H
