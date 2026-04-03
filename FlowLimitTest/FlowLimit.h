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

class CircularBuffer
{
private:
    QVector<double> xData, yData;
    int capacity;
    int start = 0;   // 起始索引
    int count = 0;   // 当前数据量

public:
    int yMin{-1};
    int yMax{10};

public:
    CircularBuffer(int size) : capacity(size)
    {
        xData.resize(size);
        yData.resize(size);
    }

    void addPoint(double x, double y)
    {
        int writeIndex = (start + count) % capacity;
        xData[writeIndex] = x;
        yData[writeIndex] = y;

        if (count < capacity) {
            count++;
        } else {
            // 缓冲区已满，移动起始位置
            start = (start + 1) % capacity;
        }
    }

    QVector<double> getXData() const
    {
        QVector<double> result;
        result.reserve(count);
        for (int i = 0; i < count; ++i) {
            int index = (start + i) % capacity;
            result.append(xData[index]);
        }
        return result;
    }

    QVector<double> getYData()
    {
        yMin = -1;
        yMax = 10;
        QVector<double> result;
        result.reserve(count);
        for (int i = 0; i < count; ++i) {
            int index = (start + i) % capacity;
            auto curY = yData[index];
            yMin = curY < yMin ? curY : yMin;
            yMax = curY > yMax ? curY : yMax;
            result.append(curY);
        }
        return result;
    }
};

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
    void handleShowData(const ShowData& data);

private:
    bool isStarted_{false};
    Server* server_{};
    Ui::FlowLimit* ui;

    QVector<double> dx_;
    QVector<double> dy_;

    int curCount_{1};
    std::shared_ptr<CircularBuffer> circleBuf_{};
    QCustomPlot* plot_{};
};
#endif   // FLOWLIMIT_H
