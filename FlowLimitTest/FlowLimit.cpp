#include "FlowLimit.h"

#include "./ui_FlowLimit.h"

FlowLimit::FlowLimit(QWidget* parent) : QMainWindow(parent), ui(new Ui::FlowLimit)
{
    ui->setupUi(this);
    BaseInit();
    UiSingnalInit();
}

FlowLimit::~FlowLimit()
{
    isRun_ = false;
    producer_->quit();

    if (pTh_->isRunning()) {
        pTh_->quit();
        pTh_->wait(2000);
    }
    if (wTh_->isRunning()) {
        wTh_->quit();
        wTh_->wait(2000);
    }

    delete ui;
}

void FlowLimit::BaseInit()
{
    isRun_ = true;
    plot_ = new QCustomPlot(this);
    auto* lay = new QVBoxLayout(ui->widget);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setSpacing(0);
    lay->addWidget(plot_);

    producer_ = new Producer(this);
    connect(this, &FlowLimit::StartTest, producer_, &Producer::start);
    connect(this, &FlowLimit::StopTest, producer_, &Producer::stop);
    connect(producer_, &Producer::produce, this, &FlowLimit::onReadyData);

    consumer_ = new Consumer(this);

    // 生产者
    pTh_ = new QThread(this);
    auto* workPro = new QObject();
    workPro->moveToThread(pTh_);
    connect(pTh_, &QThread::started, workPro, [this]() { producer_->produceTask(); });
    connect(pTh_, &QThread::finished, pTh_, &QThread::quit);
    connect(pTh_, &QThread::finished, pTh_, &QObject::deleteLater);
    connect(pTh_, &QThread::finished, workPro, &QObject::deleteLater);
    pTh_->start();

    // 工作者
    wTh_ = new QThread(this);
    auto* workFlm = new QObject();
    workFlm->moveToThread(wTh_);
    connect(wTh_, &QThread::started, workFlm, [this]() { SenderTask(); });
    connect(wTh_, &QThread::finished, wTh_, &QThread::quit);
    connect(wTh_, &QThread::finished, wTh_, &QObject::deleteLater);
    connect(wTh_, &QThread::finished, workFlm, &QObject::deleteLater);
    wTh_->start();
}

void FlowLimit::UiSingnalInit()
{
    ui->readySize->setText("20");
    connect(ui->btnSendStart, &QPushButton::clicked, this, [this]() {
        double val = ui->readySize->text().trimmed().toDouble();
        int valInt = static_cast<int>(val * 1024 * 1024);
        emit StartTest(valInt);
    });
    connect(ui->btnSendStop, &QPushButton::clicked, this, [this]() { emit StopTest(); });
    connect(this, &FlowLimit::uiSigCurByteToWrite, this,
            [this](double val) { ui->edBytesToWrite->setText(QString::number(val)); });
    connect(this, &FlowLimit::uiSigCurDelay, this, [this](double val) { ui->curDelay->setText(QString::number(val)); });
}

void FlowLimit::onReadyData(QByteArray data)
{
    QMutexLocker locker(&dataMutex_);
    dataQueue_.enqueue(data);

    // 模拟 100 个块的发送缓存没满的不算。
    if (dataQueue_.size() > 5) {
        byteToWrite_ = dataQueue_.size() * ONE_CHUCK_SIZE;
    } else {
        byteToWrite_ = 0;
    }

    recvCount_++;
    if (recvCount_ % CYCLE_CONTRL == 0) {
        emit uiSigCurByteToWrite(byteToWrite_ * 1.0 / (1024 * 1024));
        emit uiSigCurDelay(2.0);
    }
}

void FlowLimit::SenderTask()
{
    int size{};
    QByteArray data;
    while (isRun_) {
        {
            QMutexLocker locker(&dataMutex_);
            size = dataQueue_.size();
            if (size > 0) {
                data = dataQueue_.dequeue();
            }
        }
        if (size < 1) {
            QThread::msleep(1);
            continue;
        }
        consumer_->Use(data);
    }
}