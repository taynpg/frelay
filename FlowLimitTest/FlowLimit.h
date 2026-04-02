#ifndef FLOWLIMIT_H
#define FLOWLIMIT_H

#include <QMainWindow>
#include <QMutex>
#include <QQueue>
#include <qcustomplot.h>

#include "Consumer.h"
#include "Producer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class FlowLimit;
}
QT_END_NAMESPACE

class FlowLimit : public QMainWindow
{
    Q_OBJECT

public:
    explicit FlowLimit(QWidget* parent = nullptr);
    ~FlowLimit() override;

private:
    void BaseInit();
    void UiSingnalInit();

signals:
    void StartTest(int size);
    void StopTest();
    void uiSigCurByteToWrite(double val);
    void uiSigCurDelay(double val);

public slots:
    void onReadyData(QByteArray data);
    
private:
    void SenderTask();

private:
    Ui::FlowLimit* ui;
    QCustomPlot* plot_{};
    Producer* producer_{};
    Consumer* consumer_{};
    bool isRun_{};
    unsigned int recvCount_{};
    int byteToWrite_{};
    QMutex dataMutex_{};
    QThread* pTh_{};
    QThread* wTh_{};
    QQueue<QByteArray> dataQueue_{};
};
#endif   // FLOWLIMIT_H
