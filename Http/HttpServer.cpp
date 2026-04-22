// HttpServer.cpp
#include "HttpServer.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

HttpServer::HttpServer(const QString& directory) : server(std::make_unique<httplib::Server>()), root_dir(directory)
{
    ensureRootDirectory();
}

bool HttpServer::start(int port)
{
    setupRoutes();

    std::cout << "==========================================" << std::endl;
    std::cout << "文件下载服务启动成功！" << std::endl;
    std::cout << "访问地址: http://localhost:" << port << std::endl;
    std::cout << "文件目录: " << root_dir.toStdString() << std::endl;
    std::cout << "==========================================" << std::endl;
    return server->listen("0.0.0.0", port);
}

void HttpServer::stop()
{
    if (server) {
        server->stop();
    }
}

bool HttpServer::is_running() const
{
    return server && server->is_running();
}

void HttpServer::ensureRootDirectory() const
{
    QDir dir(root_dir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

std::string HttpServer::formatFileSize(qint64 size) const
{
    if (size < 1024) {
        return std::to_string(size) + " B";
    } else if (size < 1024 * 1024) {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.1f KB", size / 1024.0);
        return buffer;
    } else {
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%.1f MB", size / (1024.0 * 1024.0));
        return buffer;
    }
}

std::string HttpServer::formatFolderSize() const
{
    return "[文件夹]";
}

std::string HttpServer::getContentType(const QString& filename) const
{
    QString file = filename.toLower();

    if (file.endsWith(".txt"))
        return "text/plain";
    if (file.endsWith(".html") || file.endsWith(".htm"))
        return "text/html";
    if (file.endsWith(".css"))
        return "text/css";
    if (file.endsWith(".js"))
        return "application/javascript";

    if (file.endsWith(".jpg") || file.endsWith(".jpeg"))
        return "image/jpeg";
    if (file.endsWith(".png"))
        return "image/png";
    if (file.endsWith(".gif"))
        return "image/gif";
    if (file.endsWith(".bmp"))
        return "image/bmp";
    if (file.endsWith(".ico"))
        return "image/x-icon";

    if (file.endsWith(".pdf"))
        return "application/pdf";
    if (file.endsWith(".zip"))
        return "application/zip";
    if (file.endsWith(".rar"))
        return "application/x-rar-compressed";
    if (file.endsWith(".7z"))
        return "application/x-7z-compressed";

    if (file.endsWith(".mp3"))
        return "audio/mpeg";
    if (file.endsWith(".wav"))
        return "audio/wav";

    if (file.endsWith(".mp4"))
        return "video/mp4";
    if (file.endsWith(".avi"))
        return "video/x-msvideo";

    if (file.endsWith(".doc") || file.endsWith(".docx"))
        return "application/msword";
    if (file.endsWith(".xls") || file.endsWith(".xlsx"))
        return "application/vnd.ms-excel";
    if (file.endsWith(".ppt") || file.endsWith(".pptx"))
        return "application/vnd.ms-powerpoint";

    return "application/octet-stream";
}

void HttpServer::setupRoutes()
{
    // 首页
    server->Get("/", [this](const httplib::Request& req, httplib::Response& res) { handleRoot(req, res); });

    // 文件列表页面（根目录）
    server->Get("/files", [this](const httplib::Request& req, httplib::Response& res) { handleFileList(req, res, ""); });

    // 子目录列表
    server->Get("/list(.*)", [this](const httplib::Request& req, httplib::Response& res) {
        std::string path = req.matches[1];
        if (path.empty() || path == "/") {
            path = "";
        } else if (path[0] == '/') {
            path = path.substr(1);
        }
        handleFileList(req, res, path);
    });

    // 下载文件
    server->Get("/download/(.*)", [this](const httplib::Request& req, httplib::Response& res) { handleDownload(req, res); });

    // 健康检查
    server->Get("/health", [this](const httplib::Request& req, httplib::Response& res) { handleHealth(req, res); });
}

void HttpServer::handleRoot(const httplib::Request& req, httplib::Response& res)
{
    std::string html = R"(<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title>文件下载服务</title>
<style>
body {
    font-family: Arial, sans-serif;
    margin: 0;
    padding: 20px;
    background: #f0f0f0;
    color: #333;
}
.container {
    max-width: 800px;
    margin: 0 auto;
    background: white;
    padding: 20px;
    border: 1px solid #ccc;
    border-radius: 5px;
}
h1 {
    color: #0066cc;
    border-bottom: 2px solid #0066cc;
    padding-bottom: 10px;
}
h2 {
    color: #333;
    border-left: 4px solid #0066cc;
    padding-left: 10px;
}
.nav {
    margin: 20px 0;
    padding: 15px;
    background: #e8f3ff;
    border: 1px solid #b3d9ff;
    border-radius: 3px;
}
.nav a {
    display: inline-block;
    margin-right: 10px;
    padding: 8px 16px;
    background: #0066cc;
    color: white;
    text-decoration: none;
    border-radius: 3px;
}
.nav a:hover {
    background: #0052a3;
}
.feature-item {
    background: #f9f9f9;
    margin: 10px 0;
    padding: 15px;
    border: 1px solid #ddd;
    border-radius: 3px;
}
code {
    background: #f5f5f5;
    padding: 2px 4px;
    border: 1px solid #ddd;
    font-family: "Courier New", monospace;
}
.footer {
    margin-top: 20px;
    padding-top: 10px;
    border-top: 1px solid #ccc;
    color: #666;
    font-size: 12px;
    text-align: center;
}
table {
    width: 100%;
    border-collapse: collapse;
    margin: 10px 0;
}
table, th, td {
    border: 1px solid #ccc;
}
th, td {
    padding: 8px;
    text-align: left;
}
th {
    background: #e8f3ff;
}
.folder-icon {
    color: #ff9900;
}
.file-icon {
    color: #0066cc;
}
.breadcrumb {
    margin: 10px 0;
    padding: 5px 10px;
    background: #f0f0f0;
    border: 1px solid #ddd;
    border-radius: 3px;
}
</style>
</head>
<body>
<div class="container">
<h1>📁 文件下载服务</h1>
<p>这是一个简单的HTTP文件服务器，兼容各种旧版本浏览器。</p>

<div class="nav">
    <a href="/files">查看文件列表</a>
    <a href="/health">服务状态</a>
</div>

<h2>主要功能</h2>
<div class="feature-item">
    <strong>📁 文件/文件夹列表</strong><br>
    查看服务器上所有文件和文件夹。<br>
    地址: <a href="/files"><code>/files</code></a>
</div>

<div class="feature-item">
    <strong>⬇️ 文件下载</strong><br>
    点击文件名可直接下载文件。<br>
    格式: <code>/download/路径/文件名</code>
</div>

<div class="feature-item">
    <strong>📄 文件浏览</strong><br>
    支持浏览子目录，面包屑导航。<br>
    格式: <code>/list/子目录</code>
</div>

<h2>使用示例</h2>
<pre>
# 根目录列表
http://localhost:8080/files

# 子目录列表
http://localhost:8080/list/subfolder

# 下载文件
http://localhost:8080/download/path/to/file.txt
</pre>

<h2>文件类型支持</h2>
<table>
<tr><th>文件类型</th><th>说明</th></tr>
<tr><td>📁 文件夹</td><td>可点击进入子目录</td></tr>
<tr><td>.txt</td><td>文本文件</td></tr>
<tr><td>.jpg/.png/.gif</td><td>图片文件</td></tr>
<tr><td>.pdf</td><td>PDF文档</td></tr>
<tr><td>.zip/.rar</td><td>压缩文件</td></tr>
<tr><td>.mp3/.wav</td><td>音频文件</td></tr>
<tr><td>其他</td><td>二进制下载</td></tr>
</table>

<div class="footer">
<p>服务器根目录: )" + root_dir.toStdString() +
                       R"(</p>
<p>兼容性: 支持IE6+、Firefox 2+、Chrome 1+、Safari 3+等旧版浏览器</p>
</div>
</div>
</body>
</html>)";
    res.set_content(html, "text/html");
}

void HttpServer::handleFileList(const httplib::Request& req, httplib::Response& res, const std::string& relativePath)
{
    QString currentPath = QDir(root_dir).filePath(QString::fromStdString(relativePath));
    QDir dir(currentPath);

    if (!dir.exists()) {
        res.status = 404;
        std::string html = R"(<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title>目录不存在</title>
<style>
body { font-family: Arial, sans-serif; margin: 50px; text-align: center; }
.error { color: #d9534f; font-size: 24px; margin: 20px; }
.back { margin: 20px; }
</style>
</head>
<body>
<div class="error">⚠️ 目录不存在: )" +
                           relativePath + R"(</div>
<div class="back"><a href="/files">返回根目录</a></div>
</body>
</html>)";
        res.set_content(html, "text/html");
        return;
    }

    // 获取文件夹和文件列表
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    dir.setSorting(QDir::DirsFirst | QDir::Name);

    std::stringstream ss;
    ss << "<!DOCTYPE html>"
       << "<html><head>"
       << "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
       << "<title>文件列表 - " << (relativePath.empty() ? "根目录" : relativePath) << "</title>"
       << "<style>"
       << "body { font-family: Arial, sans-serif; margin: 20px; padding: 0; background: #f0f0f0; }"
       << ".container { max-width: 900px; margin: 0 auto; background: white; padding: 20px; border: 1px solid #ccc; }"
       << "h1 { color: #0066cc; border-bottom: 2px solid #0066cc; padding-bottom: 10px; }"
       << ".breadcrumb { margin: 10px 0; padding: 8px 12px; background: #f8f9fa; border: 1px solid #dee2e6; border-radius: 3px; }"
       << ".breadcrumb a { color: #0066cc; text-decoration: none; }"
       << ".breadcrumb a:hover { text-decoration: underline; }"
       << ".breadcrumb span { color: #666; }"
       << ".nav-links { margin: 10px 0; }"
       << ".nav-links a { margin-right: 10px; color: #0066cc; text-decoration: none; }"
       << ".nav-links a:hover { text-decoration: underline; }"
       << ".list-table { width: 100%; border-collapse: collapse; margin: 20px 0; }"
       << ".list-table th, .list-table td { border: 1px solid #ccc; padding: 8px; text-align: left; }"
       << ".list-table th { background: #e8f3ff; font-weight: bold; }"
       << ".list-table tr:nth-child(even) { background: #f9f9f9; }"
       << ".list-table tr:hover { background: #f0f7ff; }"
       << ".folder-row { background: #fff8e1 !important; }"
       << ".folder-row:hover { background: #ffecb3 !important; }"
       << ".folder-icon { color: #ff9800; font-weight: bold; }"
       << ".file-icon { color: #2196f3; }"
       << ".item-name a { color: #0066cc; text-decoration: none; }"
       << ".item-name a:hover { text-decoration: underline; }"
       << ".item-size { color: #666; font-size: 0.9em; }"
       << ".item-date { color: #666; font-size: 0.9em; }"
       << ".item-action { min-width: 60px; }"
       << ".no-items { padding: 20px; background: #f9f9f9; border: 1px dashed #ccc; text-align: center; }"
       << ".current-dir { color: #666; font-size: 0.9em; margin: 5px 0; }"
       << "</style>"
       << "</head><body>"
       << "<div class=\"container\">"
       << "<h1>📁 文件列表</h1>";

    // 面包屑导航
    ss << "<div class=\"breadcrumb\">";
    ss << "<a href=\"/\">首页</a> &gt; ";
    ss << "<a href=\"/files\">根目录</a>";

    if (!relativePath.empty()) {
        std::vector<std::string> pathParts;
        std::stringstream pathStream(relativePath);
        std::string part;

        // 分割路径
        while (std::getline(pathStream, part, '/')) {
            if (!part.empty()) {
                pathParts.push_back(part);
            }
        }

        // 构建面包屑
        std::string accumulatedPath;
        for (size_t i = 0; i < pathParts.size(); i++) {
            accumulatedPath += "/" + pathParts[i];
            ss << " &gt; <a href=\"/list" << accumulatedPath << "\">" << pathParts[i] << "</a>";
        }
    }
    ss << "</div>";

    ss << "<div class=\"nav-links\">"
       << "<a href=\"/\">← 返回首页</a> | "
       << "<a href=\"/files\">↻ 刷新列表</a>"
       << "</div>";

    // 当前目录信息
    ss << "<div class=\"current-dir\">当前目录: " << (relativePath.empty() ? "/" : relativePath.c_str()) << "</div>";

    QFileInfoList items = dir.entryInfoList();

    if (items.isEmpty()) {
        ss << "<div class=\"no-items\">目录为空，没有文件或文件夹。</div>";
    } else {
        ss << "<h2>目录内容 (" << items.size() << " 项)</h2>"
           << "<table class=\"list-table\">"
           << "<tr><th width=\"50%\">名称</th><th width=\"15%\">大小</th><th width=\"20%\">修改时间</th><th "
              "width=\"15%\">操作</th></tr>";

        for (const QFileInfo& item : items) {
            QString name = item.fileName();
            QString fullPath = item.absoluteFilePath();
            QString relativeFilePath = QDir(root_dir).relativeFilePath(fullPath);

            bool isDir = item.isDir();

            ss << "<tr" << (isDir ? " class=\"folder-row\"" : "") << ">";

            // 名称列
            ss << "<td class=\"item-name\">";
            if (isDir) {
                ss << "<span class=\"folder-icon\">📁 </span>";
                // 文件夹链接
                std::string folderUrl = "/list";
                if (!relativePath.empty()) {
                    folderUrl += "/" + relativePath;
                }
                folderUrl += "/" + name.toStdString();
                // 编码 URL
                std::string encodedUrl;
                for (char c : folderUrl) {
                    if (c == ' ')
                        encodedUrl += "%20";
                    else
                        encodedUrl += c;
                }
                ss << "<a href=\"" << encodedUrl << "\">" << name.toStdString() << "/</a>";
            } else {
                ss << "<span class=\"file-icon\">📄 </span>";
                // 文件链接
                std::string fileUrl = "/download/" + relativeFilePath.toStdString();
                // 编码 URL
                std::string encodedUrl;
                for (char c : fileUrl) {
                    if (c == ' ')
                        encodedUrl += "%20";
                    else
                        encodedUrl += c;
                }
                ss << "<a href=\"" << encodedUrl << "\">" << name.toStdString() << "</a>";
            }
            ss << "</td>";

            // 大小列
            ss << "<td class=\"item-size\">";
            if (isDir) {
                ss << this->formatFolderSize();
            } else {
                ss << this->formatFileSize(item.size());
            }
            ss << "</td>";

            // 修改时间列
            ss << "<td class=\"item-date\">" << item.lastModified().toString("yyyy-MM-dd hh:mm").toStdString() << "</td>";

            // 操作列
            ss << "<td class=\"item-action\">";
            if (isDir) {
                ss << "<a href=\"/list";
                if (!relativePath.empty()) {
                    ss << "/" << relativePath;
                }
                ss << "/" << name.toStdString() << "\">打开</a>";
            } else {
                ss << "<a href=\"/download/" << relativeFilePath.toStdString() << "\">下载</a>";
            }
            ss << "</td>";

            ss << "</tr>";
        }

        ss << "</table>";
    }

    // 统计信息
    int dirCount = 0;
    int fileCount = 0;
    qint64 totalSize = 0;

    for (const QFileInfo& item : items) {
        if (item.isDir()) {
            dirCount++;
        } else {
            fileCount++;
            totalSize += item.size();
        }
    }

    ss << "<div style=\"margin-top: 20px; padding: 10px; background: #f8f9fa; border: 1px solid #dee2e6;\">"
       << "<strong>统计信息:</strong> " << dirCount << " 个文件夹, " << fileCount << " 个文件, "
       << "总大小: " << this->formatFileSize(totalSize) << "</div>";

    ss << "</div></body></html>";

    res.set_content(ss.str(), "text/html");
}

void HttpServer::handleDownload(const httplib::Request& req, httplib::Response& res)
{
    if (req.matches.size() < 2) {
        res.status = 400;
        res.set_content("文件路径未指定", "text/plain");
        return;
    }

    std::string filepath = req.matches[1];
    QString fullPath = QDir(root_dir).filePath(QString::fromStdString(filepath));

    QFile file(fullPath);
    if (!file.exists()) {
        res.status = 404;

        // 返回友好的404页面
        std::string html = R"(<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title>文件未找到</title>
<style>
body { font-family: Arial, sans-serif; margin: 50px; text-align: center; }
.error { color: #d9534f; font-size: 24px; margin: 20px; }
.back { margin: 20px; }
</style>
</head>
<body>
<div class="error">⚠️ 文件未找到: )" +
                           filepath + R"(</div>
<div class="back"><a href="/files">返回文件列表</a></div>
</body>
</html>)";
        res.set_content(html, "text/html");
        return;
    }

    QFileInfo fileInfo(file);
    if (fileInfo.isDir()) {
        res.status = 400;
        res.set_content("不能下载文件夹", "text/plain");
        return;
    }

    // 获取文件大小
    qint64 fileSize = fileInfo.size();

    // 设置 Content-Type
    std::string content_type = getContentType(fileInfo.fileName());
    res.set_header("Content-Type", content_type);

    // 设置下载头
    std::string filename = fileInfo.fileName().toStdString();
    res.set_header("Content-Disposition", "attachment; filename=\"" + filename + "\"");

    // 设置文件大小
    res.set_header("Content-Length", std::to_string(fileSize));

    // 普通下载
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray fileData = file.readAll();
        file.close();

        res.set_content(std::string(fileData.constData(), fileData.size()), content_type);
    } else {
        res.status = 500;
        res.set_content("无法读取文件", "text/plain");
    }
}

void HttpServer::handleHealth(const httplib::Request& req, httplib::Response& res)
{
    std::string html = R"(<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<title>服务状态</title>
<style>
body { font-family: Arial, sans-serif; margin: 50px; text-align: center; }
.status { font-size: 24px; color: #28a745; margin: 20px; }
.info { margin: 20px; padding: 20px; background: #f8f9fa; border: 1px solid #dee2e6; display: inline-block; }
</style>
</head>
<body>
<div class="status">✅ 服务运行正常</div>
<div class="info">
<p><strong>服务器状态:</strong> 运行中</p>
<p><strong>文件根目录:</strong> )" +
                       root_dir.toStdString() + R"(</p>
</div>
<div style="margin: 20px;">
<a href="/">返回首页</a> |
<a href="/files">文件列表</a>
</div>
</body>
</html>)";

    res.set_content(html, "text/html");
}
