#ifndef FLOWLIMIT_H
#define FLOWLIMIT_H

#include <QMainWindow>
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
    explicit FlowLimit(QWidget *parent = nullptr);
    ~FlowLimit() override;

private:
    void BaseInit();

private:
    Ui::FlowLimit *ui;
    QCustomPlot* plot_{};
};
#endif // FLOWLIMIT_H
