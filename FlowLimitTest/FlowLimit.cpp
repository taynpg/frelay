#include "FlowLimit.h"
#include <Util.h>
#include <QMessageBox>

#include "./ui_FlowLimit.h"

FlowLimit::FlowLimit(QWidget* parent) : QMainWindow(parent), ui(new Ui::FlowLimit)
{
    ui->setupUi(this);

    Util::InitLogger("frelayServer.log", "frelayServer");
    qInstallMessageHandler(Util::ConsoleMsgHander);

    server_ = new Server(this);

    BaseInit();
    UiSingnalInit();
}

FlowLimit::~FlowLimit()
{
    delete ui;
}

void FlowLimit::BaseInit()
{
    plot_ = new QCustomPlot(this);
    auto* lay = new QVBoxLayout(ui->widget);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(plot_);
}

void FlowLimit::UiSingnalInit()
{

}

void FlowLimit::startMonitor()
{
    if (!server_->startServer(9009)) {
        QMessageBox::information(this, "提示", "=====> 启动失败。");
        isStarted_ = false;
    }
    else {
        isStarted_ = true;
    }
}