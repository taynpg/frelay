#include "CompareControl.h"

#include "ui_CompareControl.h"

Compare::Compare(QWidget* parent) : QWidget(parent), ui(new Ui::Compare)
{
    ui->setupUi(this);
    InitControl();
}

Compare::~Compare()
{
    delete ui;
}

void Compare::InitControl()
{
    InitTabWidget();
}

void Compare::InitTabWidget()
{
     QStringList headers;
     headers << tr("") << tr("LocalPath") << tr("RemotePath");
     ui->tableWidget->setColumnCount(headers.size());
     ui->tableWidget->setHorizontalHeaderLabels(headers);
     ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

     ui->comboBox->setEditable(true);
    // ui->tableWidget->setColumnWidth(0, 50);
     ui->tableWidget->setColumnWidth(1, 500);
     ui->tableWidget->setColumnWidth(2, 500);

     ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
     ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
     ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
     ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
     ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    ui->tableWidget->setDragEnabled(true);
    ui->tableWidget->viewport()->setAcceptDrops(true);
    ui->tableWidget->setDropIndicatorShown(true);
    ui->tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tableWidget->setDragDropMode(QAbstractItemView::DragDrop);
}
