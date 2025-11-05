#include "Console.h"

ConsoleHelper::ConsoleHelper(QObject* parent) : QObject(parent)
{
}

ConsoleHelper::~ConsoleHelper()
{
}

void ConsoleHelper::RunWorker(ClientCore* clientCore)
{
    clientCore_ = clientCore;

    sockWorker_ = new SocketWorker(clientCore_, nullptr);
    heatBeat_ = new HeatBeat(clientCore_);

    clientCore_->moveToThread(sockWorker_);
    fileTrans_ = new FileTrans(clientCore_);
    
    connect(clientCore_, &ClientCore::conSuccess, this, [this]() { qInfo() << QString(tr("Connected.")); });
    connect(clientCore_, &ClientCore::conFailed, this, [this]() { qInfo() << QString(tr("Connect failed.")); });
    connect(clientCore_, &ClientCore::connecting, this, [this]() { qInfo() << QString(tr("Connecting...")); });
    connect(clientCore_, &ClientCore::sigDisconnect, this, [this]() { qInfo() << QString(tr("Disconnected.")); });
    connect(this, &ConsoleHelper::sigDoConnect, clientCore_, &ClientCore::DoConnect);
    connect(sockWorker_, &QThread::finished, sockWorker_, &QObject::deleteLater);

    sockWorker_->start();
    heatBeat_->start();
}

void ConsoleHelper::SetIpPort(const QString& ip, quint16 port)
{
    ip_ = ip;
    port_ = port;
    qInfo() << "SetIpPort:" << ip_ << port_;
}

void ConsoleHelper::Connect()
{
    emit sigDoConnect(ip_, port_);
}
