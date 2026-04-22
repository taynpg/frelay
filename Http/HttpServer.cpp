#include "HttpServer.h"

#include <QDateTime>
#include <QNetworkInterface>
#include <QTextStream>

HttpServer::HttpServer(QObject* parent) : QTcpServer(parent), m_port(0)
{
    // 获取本机IP
    QString localIp = "127.0.0.1";
    foreach (const QHostAddress& address, QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != QHostAddress(QHostAddress::LocalHost)) {
            QString addrStr = address.toString();
            // 优先使用以192.168、10.、172.16-31开头的局域网IP
            if (addrStr.startsWith("192.168.") || addrStr.startsWith("10.") || addrStr.startsWith("172.16.") ||
                addrStr.startsWith("172.17.") || addrStr.startsWith("172.18.") || addrStr.startsWith("172.19.") ||
                addrStr.startsWith("172.20.") || addrStr.startsWith("172.21.") || addrStr.startsWith("172.22.") ||
                addrStr.startsWith("172.23.") || addrStr.startsWith("172.24.") || addrStr.startsWith("172.25.") ||
                addrStr.startsWith("172.26.") || addrStr.startsWith("172.27.") || addrStr.startsWith("172.28.") ||
                addrStr.startsWith("172.29.") || addrStr.startsWith("172.30.") || addrStr.startsWith("172.31.")) {
                localIp = addrStr;
                break;
            } else if (localIp == "127.0.0.1") {
                localIp = addrStr;
            }
        }
    }
    m_localIp = localIp;
}

HttpServer::~HttpServer()
{
    stop();
}

bool HttpServer::start(quint16 port, const QString& shareDir)
{
    m_port = port;

    // 规范化共享目录路径
    QDir dir(shareDir);
    if (!dir.exists()) {
        qWarning() << "Share directory does not exist:" << shareDir;
        qWarning() << "Current directory will be used instead.";
        m_shareDir = QDir::currentPath();
    } else {
        m_shareDir = dir.absolutePath();
    }

    qDebug() << "Using share directory:" << m_shareDir;

    if (!this->listen(QHostAddress::Any, port)) {
        qWarning() << "Failed to start server on port" << port;
        return false;
    }

    qDebug() << "";
    qDebug() << "========================================";
    qDebug() << "HTTP Server started successfully!";
    qDebug() << "Share directory: " << m_shareDir;
    qDebug() << "Access URLs:";
    qDebug() << "  Local:  http://127.0.0.1:" << port;
    qDebug() << "  Network: http://" << m_localIp << ":" << port;
    qDebug() << "========================================";
    qDebug() << "Press Ctrl+C to stop";
    qDebug() << "";

    return true;
}

void HttpServer::stop()
{
    if (isListening()) {
        close();
        qDebug() << "HTTP Server stopped";
    }

    // 断开所有客户端连接
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        it.key()->close();
        it.key()->deleteLater();
    }
    m_clients.clear();
}

QString HttpServer::getServerUrl() const
{
    return QString("http://%1:%2").arg(m_localIp).arg(m_port);
}

void HttpServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* clientSocket = new QTcpSocket(this);
    if (!clientSocket->setSocketDescriptor(socketDescriptor)) {
        delete clientSocket;
        return;
    }

    ClientConnection connection;
    connection.socket = clientSocket;
    m_clients[clientSocket] = connection;

    connect(clientSocket, &QTcpSocket::readyRead, this, &HttpServer::onClientReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &HttpServer::onClientDisconnected);

    QString clientAddr = clientSocket->peerAddress().toString();
    if (clientAddr.startsWith("::ffff:")) {
        clientAddr = clientAddr.mid(7);   // 移除IPv6前缀
    }
    qDebug() << "New client connected:" << clientAddr;
}

void HttpServer::onClientReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !m_clients.contains(socket))
        return;

    ClientConnection& connection = m_clients[socket];
    connection.buffer.append(socket->readAll());

    // 检查是否接收到完整的HTTP请求（以空行结束）
    if (connection.buffer.contains("\r\n\r\n")) {
        QString requestedPath = parseRequestPath(connection.buffer);

        // 记录请求
        QString clientAddr = socket->peerAddress().toString();
        if (clientAddr.startsWith("::ffff:")) {
            clientAddr = clientAddr.mid(7);
        }
        qDebug() << "Request:" << requestedPath << "from" << clientAddr;

        // 特殊处理 favicon.ico
        if (requestedPath == "/favicon.ico") {
            // 返回一个空的favicon响应
            sendHttpResponse(socket, 204, "No Content");
            connection.buffer.clear();
            return;
        }

        // 验证路径安全性
        QString absolutePath;
        if (isValidPath(requestedPath, absolutePath)) {
            QFileInfo fileInfo(absolutePath);

            if (fileInfo.isDir()) {
                // 如果是目录，返回目录列表
                sendDirectoryListing(socket, absolutePath, requestedPath);
            } else if (fileInfo.isFile() && fileInfo.isReadable()) {
                // 如果是文件，发送文件
                sendFile(socket, absolutePath);
            } else {
                send404(socket, requestedPath);
            }
        } else {
            send400(socket, "Invalid path: " + requestedPath);
        }

        // 清空缓冲区
        connection.buffer.clear();
    }
}

void HttpServer::onClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket && m_clients.contains(socket)) {
        m_clients.remove(socket);
        socket->deleteLater();
    }
}

QString HttpServer::parseRequestPath(const QByteArray& requestData)
{
    QString request = QString::fromUtf8(requestData);

    // 查找请求行
    int lineEnd = request.indexOf("\r\n");
    if (lineEnd == -1)
        return "/";

    QString requestLine = request.left(lineEnd);

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    QStringList parts = requestLine.split(' ', Qt::SkipEmptyParts);
#else
    QStringList parts = requestLine.split(' ', QString::SkipEmptyParts);
#endif

    if (parts.size() < 2)
        return "/";

    QString path = parts[1];

    // 处理URL编码
    path = QUrl::fromPercentEncoding(path.toUtf8());

    // 处理查询字符串
    int queryIndex = path.indexOf('?');
    if (queryIndex != -1) {
        path = path.left(queryIndex);
    }

    // 确保路径以斜杠开头
    if (!path.startsWith("/")) {
        path = "/" + path;
    }

    return path;
}

bool HttpServer::isValidPath(const QString& requestedPath, QString& absolutePath)
{
    // 移除开头的斜杠
    QString cleanRequest = requestedPath;
    if (cleanRequest.startsWith("/")) {
        cleanRequest = cleanRequest.mid(1);
    }

    // 规范化请求路径
    cleanRequest = QDir::cleanPath(cleanRequest);

    // 防止路径穿越攻击
    if (cleanRequest.startsWith("..") || cleanRequest.contains("/..") || cleanRequest.contains("../") || cleanRequest == "..") {
        return false;
    }

    // 构建绝对路径
    QDir baseDir(m_shareDir);
    absolutePath = baseDir.absoluteFilePath(cleanRequest);

    // 规范化绝对路径
    absolutePath = QDir::cleanPath(absolutePath);

    // 确保路径在共享目录内
    if (!absolutePath.startsWith(m_shareDir)) {
        return false;
    }

    return true;
}

void HttpServer::sendFile(QTcpSocket* socket, const QString& filePath)
{
    QFile file(filePath);
    if (!file.exists()) {
        send404(socket, QFileInfo(filePath).fileName());
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        sendHttpResponse(socket, 403, "Forbidden",
                         QString("<html><body><h1>403 Forbidden</h1><p>Permission denied</p></body></html>").toUtf8(),
                         "text/html");
        return;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QString fileName = QFileInfo(filePath).fileName();
    QString mimeType = getMimeType(fileName);

    // 设置响应头
    QByteArray response;
    response.append("HTTP/1.1 200 OK\r\n");
    response.append("Content-Type: " + mimeType.toUtf8() + "\r\n");

    // 对于某些文件类型，直接显示而不是下载
    if (mimeType.startsWith("text/") || mimeType.startsWith("image/") || mimeType.startsWith("application/pdf") ||
        mimeType == "application/json" || mimeType == "application/xml") {
        response.append("Content-Disposition: inline; filename=\"" + QUrl::toPercentEncoding(fileName) + "\"\r\n");
    } else {
        response.append("Content-Disposition: attachment; filename=\"" + QUrl::toPercentEncoding(fileName) + "\"\r\n");
    }

    response.append("Content-Length: " + QByteArray::number(fileData.size()) + "\r\n");
    response.append("Connection: close\r\n");
    response.append("\r\n");
    response.append(fileData);

    socket->write(response);
    socket->flush();
    socket->waitForBytesWritten(3000);
    socket->disconnectFromHost();
}

void HttpServer::sendDirectoryListing(QTcpSocket* socket, const QString& dirPath, const QString& requestPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        send404(socket, requestPath);
        return;
    }

    dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    dir.setSorting(QDir::DirsFirst | QDir::Name);

    QFileInfoList entries = dir.entryInfoList();

    QString formattedPath = formatDirectoryPath(requestPath);
    QString parentPath = requestPath;
    if (parentPath != "/") {
        if (parentPath.endsWith("/")) {
            parentPath.chop(1);
        }
        int lastSlash = parentPath.lastIndexOf("/");
        if (lastSlash > 0) {
            parentPath = parentPath.left(lastSlash);
        } else {
            parentPath = "/";
        }
    }

    QString html = "<!DOCTYPE html>";
    html += "<html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<title>文件列表 - " + formattedPath + "</title>";
    html += "<style>";
    html +=
        "body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }";
    html += ".container { max-width: 1200px; margin: 0 auto; background: white; border-radius: 8px; box-shadow: 0 2px 10px "
            "rgba(0,0,0,0.1); padding: 20px; }";
    html += "h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }";
    html += ".info { background: #e8f5e9; border-left: 4px solid #4CAF50; padding: 10px; margin: 15px 0; border-radius: 4px; }";
    html += ".file-list { width: 100%; border-collapse: collapse; margin-top: 20px; }";
    html += ".file-list th { background: #4CAF50; color: white; padding: 12px; text-align: left; }";
    html += ".file-list td { padding: 12px; border-bottom: 1px solid #ddd; }";
    html += ".file-list tr:hover { background: #f5f5f5; }";
    html += ".dir { color: #2196F3; }";
    html += ".file { color: #666; }";
    html += ".size { color: #888; font-size: 0.9em; }";
    html += ".date { color: #888; font-size: 0.9em; }";
    html += ".nav { margin: 20px 0; }";
    html += ".nav a { display: inline-block; padding: 8px 16px; background: #4CAF50; color: white; text-decoration: none; "
            "border-radius: 4px; margin-right: 10px; }";
    html += ".nav a:hover { background: #45a049; }";
    html += ".icon { margin-right: 8px; }";
    html += "footer { margin-top: 20px; text-align: center; color: #888; font-size: 0.9em; }";
    html += "</style>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>📁 文件列表: " + formattedPath + "</h1>";
    html += "<div class='info'>";
    html += "<p><strong>共享目录:</strong> " + m_shareDir + "</p>";
    html += "<p><strong>服务器地址:</strong> http://" + m_localIp + ":" + QString::number(m_port) + "</p>";
    html += "</div>";

    html += "<div class='nav'>";
    if (requestPath != "/") {
        html += "<a href='" + parentPath + "'><span class='icon'>⬆️</span>返回上级</a>";
    }
    html += "<a href='/'>🏠 返回首页</a>";
    html += "</div>";

    html += "<table class='file-list'>";
    html += "<thead><tr>";
    html += "<th>名称</th><th>类型</th><th>大小</th><th>修改时间</th>";
    html += "</tr></thead><tbody>";

    if (entries.isEmpty()) {
        html += "<tr><td colspan='4' style='text-align:center;color:#888;'>目录为空</td></tr>";
    } else {
        foreach (const QFileInfo& info, entries) {
            QString name = info.fileName();
            QString urlPath = requestPath;
            if (!urlPath.endsWith("/"))
                urlPath += "/";
            urlPath += QUrl::toPercentEncoding(name);

            QString typeIcon, typeText;
            qint64 size = info.size();
            QString sizeText;

            if (info.isDir()) {
                typeIcon = "📁";
                typeText = "文件夹";
                sizeText = "-";
            } else {
                typeIcon = "📄";
                typeText = "文件";
                if (size < 1024) {
                    sizeText = QString("%1 B").arg(size);
                } else if (size < 1024 * 1024) {
                    sizeText = QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
                } else if (size < 1024 * 1024 * 1024) {
                    sizeText = QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
                } else {
                    sizeText = QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 2);
                }
            }

            QString modified = info.lastModified().toString("yyyy-MM-dd HH:mm:ss");
            QString className = info.isDir() ? "dir" : "file";

            html += "<tr>";
            html += "<td class='" + className + "'><span class='icon'>" + typeIcon + "</span><a href='" + urlPath + "'>" + name +
                    "</a></td>";
            html += "<td>" + typeText + "</td>";
            html += "<td class='size'>" + sizeText + "</td>";
            html += "<td class='date'>" + modified + "</td>";
            html += "</tr>";
        }
    }

    html += "</tbody></table>";

    html += "<footer>";
    html += "<p>Qt HTTP File Server | " + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + "</p>";
    html += "</footer>";

    html += "</div></body></html>";

    sendHttpResponse(socket, 200, "OK", html.toUtf8(), "text/html; charset=UTF-8");
}

QString HttpServer::getMimeType(const QString& fileName)
{
    static QMimeDatabase mimeDatabase;
    QMimeType mimeType = mimeDatabase.mimeTypeForFile(fileName, QMimeDatabase::MatchExtension);

    if (mimeType.isValid()) {
        return mimeType.name();
    }

    // 常见文件类型的MIME类型映射
    QString ext = QFileInfo(fileName).suffix().toLower();

    if (ext == "txt" || ext == "log" || ext == "md" || ext == "json" || ext == "xml" || ext == "yml" || ext == "yaml" ||
        ext == "ini" || ext == "cfg" || ext == "conf") {
        return "text/plain";
    }
    if (ext == "html" || ext == "htm")
        return "text/html";
    if (ext == "css")
        return "text/css";
    if (ext == "js")
        return "application/javascript";
    if (ext == "json")
        return "application/json";
    if (ext == "xml")
        return "application/xml";
    if (ext == "pdf")
        return "application/pdf";
    if (ext == "jpg" || ext == "jpeg")
        return "image/jpeg";
    if (ext == "png")
        return "image/png";
    if (ext == "gif")
        return "image/gif";
    if (ext == "bmp")
        return "image/bmp";
    if (ext == "svg")
        return "image/svg+xml";
    if (ext == "ico")
        return "image/x-icon";
    if (ext == "zip")
        return "application/zip";
    if (ext == "rar")
        return "application/vnd.rar";
    if (ext == "7z")
        return "application/x-7z-compressed";
    if (ext == "tar")
        return "application/x-tar";
    if (ext == "gz")
        return "application/gzip";
    if (ext == "exe")
        return "application/x-msdownload";
    if (ext == "msi")
        return "application/x-msi";
    if (ext == "doc" || ext == "docx")
        return "application/msword";
    if (ext == "xls" || ext == "xlsx")
        return "application/vnd.ms-excel";
    if (ext == "ppt" || ext == "pptx")
        return "application/vnd.ms-powerpoint";
    if (ext == "mp3")
        return "audio/mpeg";
    if (ext == "mp4")
        return "video/mp4";
    if (ext == "avi")
        return "video/x-msvideo";
    if (ext == "mkv")
        return "video/x-matroska";

    return "application/octet-stream";
}

void HttpServer::send404(QTcpSocket* socket, const QString& path)
{
    QString html = "<!DOCTYPE html>";
    html += "<html><head><meta charset='UTF-8'><style>";
    html += "body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }";
    html += "h1 { color: #d32f2f; }";
    html += "a { color: #1976d2; text-decoration: none; }";
    html += "</style></head><body>";
    html += "<h1>❌ 404 Not Found</h1>";
    html += "<p>请求的资源未找到: <strong>" + path + "</strong></p>";
    html += "<p><a href='/'>返回首页</a></p>";
    html += "</body></html>";

    sendHttpResponse(socket, 404, "Not Found", html.toUtf8(), "text/html; charset=UTF-8");
}

void HttpServer::send400(QTcpSocket* socket, const QString& message)
{
    QString html = "<!DOCTYPE html>";
    html += "<html><head><meta charset='UTF-8'><style>";
    html += "body { font-family: Arial, sans-serif; text-align: center; padding: 50px; }";
    html += "h1 { color: #d32f2f; }";
    html += "a { color: #1976d2; text-decoration: none; }";
    html += "</style></head><body>";
    html += "<h1>⚠️ 400 Bad Request</h1>";
    html += "<p>" + message + "</p>";
    html += "<p><a href='/'>返回首页</a></p>";
    html += "</body></html>";

    sendHttpResponse(socket, 400, "Bad Request", html.toUtf8(), "text/html; charset=UTF-8");
}

void HttpServer::sendHttpResponse(QTcpSocket* socket, int statusCode, const QString& statusText, const QByteArray& body,
                                  const QString& contentType)
{
    QByteArray response;
    response.append(QString("HTTP/1.1 %1 %2\r\n").arg(statusCode).arg(statusText).toUtf8());
    response.append("Content-Type: " + contentType.toUtf8() + "\r\n");
    response.append("Content-Length: " + QByteArray::number(body.size()) + "\r\n");
    response.append("Connection: close\r\n");
    response.append("Server: Qt-HttpServer/1.0\r\n");
    response.append("\r\n");
    response.append(body);

    socket->write(response);
    socket->flush();
    socket->waitForBytesWritten(3000);
    socket->disconnectFromHost();
}

QString HttpServer::formatDirectoryPath(const QString& path)
{
    if (path == "/")
        return "/";

    QString formatted = path;
    if (!formatted.endsWith("/")) {
        formatted += "/";
    }
    return formatted;
}