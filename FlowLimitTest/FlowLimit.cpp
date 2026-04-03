#include "FlowLimit.h"

#include <QMessageBox>
#include <Util.h>
#include <random>

#include "./ui_FlowLimit.h"

constexpr int CYCLE_BUFFER_CNT = 50;

FlowLimit::FlowLimit(QWidget* parent) : QMainWindow(parent), ui(new Ui::FlowLimit)
{
    ui->setupUi(this);

    Util::InitLogger("frelayServer.log", "frelayServer");
    qInstallMessageHandler(Util::ConsoleMsgHander);

    server_ = new Server(this);
    circleBuf_ = std::make_shared<CircularBuffer>(CYCLE_BUFFER_CNT);

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

    auto* gp = plot_->addGraph();
    gp->setPen(QPen(Qt::blue));
    gp->setBrush(QBrush(QColor(255, 255, 0)));

    // plot_->xAxis->setRange(0, 60);
    // plot_->yAxis->setRange(0, 200);
}

void FlowLimit::UiSingnalInit()
{
    connect(ui->btnStartServer, &QPushButton::clicked, this, &FlowLimit::startMonitor);
    connect(ui->btnTest, &QPushButton::clicked, this, &FlowLimit::testPlot);
    connect(server_, &Server::sigByteToWrite, this, &FlowLimit::handleShowData);
}

void FlowLimit::startMonitor()
{
    if (!server_->startServer(9009)) {
        QMessageBox::information(this, "提示", "=====> 启动失败。");
        isStarted_ = false;
    } else {
        ui->btnStartServer->setEnabled(false);
        isStarted_ = true;
    }
}

// struct ShowData {
//     unsigned int count{};
//     bool canSig{false};
//     qint64 bytesToWrite{};
//     double curDelay{};
//     std::shared_ptr<FlowController> fl;
// };

void FlowLimit::handleShowData(const ShowData& data)
{
    curCount_++;
    circleBuf_->addPoint(static_cast<double>(curCount_), data.curDelay);
    plot_->graph(0)->setData(circleBuf_->getXData(), circleBuf_->getYData());

    plot_->yAxis->setRange(circleBuf_->yMin, circleBuf_->yMax);

    int startX = (curCount_ - CYCLE_BUFFER_CNT) > 0 ? (curCount_ - CYCLE_BUFFER_CNT) : 0;
    int endX = startX + CYCLE_BUFFER_CNT;

    plot_->xAxis->setRange(startX, endX);
    plot_->replot();
}

void FlowLimit::testPlot()
{
    // 创建随机数引擎
    std::random_device rd;    // 真随机数种子
    std::mt19937 gen(rd());   // Mersenne Twister 引擎

    // 生成 [0, 100] 之间的整数
    std::uniform_int_distribution<> distrib(0, 100);
    int randomInt = distrib(gen);

    // 生成 [0.0, 1.0) 之间的浮点数
    std::uniform_real_distribution<> distribReal(0.0, 200.0);
    double randomDouble = distribReal(gen);

    dx_.append(dx_.count());
    dy_.append(randomDouble);

    plot_->graph(0)->setData(dx_, dy_);
    plot_->replot();
}