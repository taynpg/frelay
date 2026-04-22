#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <iostream>

#include "HttpServer.h"

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("Qt5 File Server");
    app.setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Simple HTTP file server for Qt5");
    parser.addHelpOption();
    parser.addVersionOption();

    // 添加端口选项
    QCommandLineOption portOption(QStringList() << "p" << "port", "Port to listen on (default: 8080)", "port", "8080");
    parser.addOption(portOption);

    // 添加目录选项
    QCommandLineOption dirOption(QStringList() << "d" << "directory", "Directory to serve (default: current directory)",
                                 "directory", ".");
    parser.addOption(dirOption);

    // 添加绑定地址选项
    QCommandLineOption hostOption(QStringList() << "b" << "bind", "Address to bind to (default: all interfaces)", "address",
                                  "0.0.0.0");
    parser.addOption(hostOption);

    parser.process(app);

    quint16 port = parser.value(portOption).toUShort();
    QString shareDir = QDir(parser.value(dirOption)).absolutePath();
    QString bindAddress = parser.value(hostOption);

    // 检查目录是否存在
    QDir dir(shareDir);
    if (!dir.exists()) {
        std::cerr << "Error: Directory does not exist: " << shareDir.toStdString() << std::endl;
        std::cerr << "Current directory: " << QDir::currentPath().toStdString() << std::endl;
        return 1;
    }

    HttpServer server;
    if (!server.start(port, shareDir)) {
        std::cerr << "Failed to start server!" << std::endl;
        return 1;
    }

    return app.exec();
}