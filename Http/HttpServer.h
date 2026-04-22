// HttpServer.h
#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <httplib.h>
#include <memory>
#include <string>

class HttpServer
{
private:
    std::unique_ptr<httplib::Server> server;
    QString root_dir;

    // 内部处理函数
    void setupRoutes();
    void handleRoot(const httplib::Request& req, httplib::Response& res);
    void handleFileList(const httplib::Request& req, httplib::Response& res, const std::string& relativePath = "");
    void handleDownload(const httplib::Request& req, httplib::Response& res);
    void handleHealth(const httplib::Request& req, httplib::Response& res);

    // 辅助函数
    std::string getContentType(const QString& filename) const;
    void ensureRootDirectory() const;
    std::string formatFileSize(qint64 size) const;
    std::string formatFolderSize() const;

public:
    // 构造函数
    HttpServer(const QString& directory = "./files");

    // 启动服务器
    bool start(int port = 8080);

    // 停止服务器
    void stop();

    // 检查服务器是否在运行
    bool is_running() const;

    // 获取服务器信息
    QString getRootDir() const
    {
        return root_dir;
    }
    void setRootDir(const QString& dir)
    {
        root_dir = dir;
        ensureRootDirectory();
    }
};

#endif   // HTTPSERVER_H
