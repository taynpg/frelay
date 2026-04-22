// main.cpp
#include <csignal>
#include <cstdlib>
#include <iostream>

#include "HttpServer.h"

#ifdef _WIN32
#include <windows.h>
#endif

std::unique_ptr<HttpServer> server;

void signalHandler(int signal)
{
    std::cout << "\n收到信号 " << signal << "，正在关闭服务器..." << std::endl;
    if (server) {
        server->stop();
    }
    exit(0);
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    // 设置信号处理
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    int port = 8080;
    QString root_dir = "./files";

    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) {
            port = std::atoi(argv[++i]);
        } else if (arg == "-d" && i + 1 < argc) {
            root_dir = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "用法: " << argv[0] << " [选项]" << std::endl;
            std::cout << "选项:" << std::endl;
            std::cout << "  -p <端口>   设置服务器端口 (默认: 8080)" << std::endl;
            std::cout << "  -d <目录>   设置文件目录 (默认: ./files)" << std::endl;
            std::cout << "  -h, --help  显示帮助信息" << std::endl;
            return 0;
        }
    }

    try {
        server = std::make_unique<HttpServer>(root_dir);

        std::cout << "启动文件下载服务..." << std::endl;
        std::cout << "按 Ctrl+C 停止服务" << std::endl;

        if (!server->start(port)) {
            std::cerr << "启动服务器失败！" << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
