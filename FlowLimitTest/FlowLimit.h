#ifndef FLOWLIMIT_H
#define FLOWLIMIT_H

#include <QMainWindow>
#include <QMutex>
#include <Server.h>
#include <qcustomplot.h>

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

public slots:
    void startMonitor();
    void testPlot();

private:
    bool isStarted_{false};
    Server* server_{};
    Ui::FlowLimit* ui;

    QVector<double> dx_;
    QVector<double> dy_;
    QCustomPlot* plot_{};
};
#endif   // FLOWLIMIT_H
