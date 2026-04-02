#include "FlowLimit.h"

#include "./ui_FlowLimit.h"

FlowLimit::FlowLimit(QWidget* parent) : QMainWindow(parent), ui(new Ui::FlowLimit)
{
    ui->setupUi(this);
    BaseInit();
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
