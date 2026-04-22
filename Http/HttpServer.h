#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

class HttpServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit HttpServer(QObject* parent = nullptr);
    ~HttpServer();

    bool start(quint16 port = 8080, const QString& shareDir = "");
    void stop();

    QString getServerUrl() const;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onClientReadyRead();
    void onClientDisconnected();

private:
    struct ClientConnection {
        QTcpSocket* socket;
        QByteArray buffer;
    };

    QHash<QTcpSocket*, ClientConnection> m_clients;
    QString m_shareDir;
    quint16 m_port;
    QString m_localIp;

    QString parseRequestPath(const QByteArray& requestData);
    void sendHttpResponse(QTcpSocket* socket, int statusCode, const QString& statusText, const QByteArray& body = QByteArray(),
                          const QString& contentType = "application/octet-stream");
    void sendFile(QTcpSocket* socket, const QString& filePath);
    bool isValidPath(const QString& requestedPath, QString& absolutePath);
    void sendDirectoryListing(QTcpSocket* socket, const QString& dirPath, const QString& requestPath);
    QString getMimeType(const QString& fileName);
    void send404(QTcpSocket* socket, const QString& path = "");
    void send400(QTcpSocket* socket, const QString& message = "");
    QString formatDirectoryPath(const QString& path);
};

#endif   // HTTPSERVER_H