#include "CompareControl.h"

#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "GuiUtil/Public.h"
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

    connect(ui->btnSave, &QPushButton::clicked, this, &Compare::Save);
    connect(ui->btnLoad, &QPushButton::clicked, this, &Compare::Load);
    connect(ui->btnLeft, &QPushButton::clicked, this, &Compare::TransToLeft);
    connect(ui->btnRight, &QPushButton::clicked, this, &Compare::TransToRight);

    LoadTitles();
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

    ui->tableWidget->setDragEnabled(false);
    ui->tableWidget->viewport()->setAcceptDrops(true);
    ui->tableWidget->setDropIndicatorShown(true);
    ui->tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tableWidget->setDragDropMode(QAbstractItemView::DropOnly);

    ui->comboBox->setFixedWidth(100);
}

void Compare::Save()
{
    auto titleKey = ui->comboBox->currentText().trimmed();
    if (titleKey.isEmpty()) {
        FTCommon::msg(this, tr("Please select or input a title."));
        return;
    }

    QFile file("CompareData.xml");
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        FTCommon::msg(this, tr("Failed to open data file."));
        return;
    }

    QString existingContent;
    QMap<QString, QStringList> dataMap;
    if (file.size() > 0) {
        existingContent = file.readAll();
        file.seek(0);
        QXmlStreamReader reader(existingContent);
        while (!reader.atEnd()) {
            if (reader.isStartElement() && reader.name() == "Entry") {
                QString key = reader.attributes().value("title").toString();
                QStringList paths;
                while (!(reader.isEndElement() && reader.name() == "Entry")) {
                    reader.readNext();
                    if (reader.isStartElement() && reader.name() == "LocalPath") {
                        paths << reader.readElementText();
                    }
                    if (reader.isStartElement() && reader.name() == "RemotePath") {
                        paths << reader.readElementText();
                    }
                }
                dataMap.insert(key, paths);
            }
            reader.readNext();
        }
    }

    QStringList paths;
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        QTableWidgetItem* localItem = ui->tableWidget->item(row, 1);
        QTableWidgetItem* remoteItem = ui->tableWidget->item(row, 2);
        paths << (localItem ? localItem->text() : "");
        paths << (remoteItem ? remoteItem->text() : "");
    }

    dataMap[titleKey] = paths;

    file.resize(0);
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("CompareData");

    for (auto it = dataMap.begin(); it != dataMap.end(); ++it) {
        writer.writeStartElement("Entry");
        writer.writeAttribute("title", it.key());

        const QStringList& pathList = it.value();
        for (int i = 0; i < pathList.size(); i += 2) {
            writer.writeTextElement("LocalPath", pathList[i]);
            writer.writeTextElement("RemotePath", pathList[i + 1]);
        }

        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();
    file.close();

    if (ui->comboBox->findText(titleKey) == -1) {
        ui->comboBox->addItem(titleKey);
    }

    FTCommon::msg(this, tr("Data saved successfully."));
}

void Compare::Load()
{
    auto titleKey = ui->comboBox->currentText().trimmed();
    if (titleKey.isEmpty()) {
        FTCommon::msg(this, tr("Please select or input a title."));
        return;
    }

    QFile file("CompareData.xml");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        FTCommon::msg(this, tr("Failed to open data file."));
        return;
    }

    QXmlStreamReader reader(&file);
    bool found = false;

    ui->tableWidget->setRowCount(0);
    while (!reader.atEnd() && !found) {
        if (reader.isStartElement() && reader.name() == "Entry") {
            if (reader.attributes().value("title").toString() == titleKey) {
                found = true;
                QStringList paths;

                while (!(reader.isEndElement() && reader.name() == "Entry")) {
                    reader.readNext();
                    if (reader.isStartElement() && reader.name() == "LocalPath") {
                        paths << reader.readElementText();
                    }
                    if (reader.isStartElement() && reader.name() == "RemotePath") {
                        paths << reader.readElementText();
                    }
                }
                for (int i = 0; i < paths.size(); i += 2) {
                    int row = ui->tableWidget->rowCount();
                    ui->tableWidget->insertRow(row);

                    QTableWidgetItem* localItem = new QTableWidgetItem(paths[i]);
                    QTableWidgetItem* remoteItem = new QTableWidgetItem(paths[i + 1]);

                    ui->tableWidget->setItem(row, 1, localItem);
                    ui->tableWidget->setItem(row, 2, remoteItem);
                }
            }
        }
        reader.readNext();
    }

    file.close();
    if (!found) {
        FTCommon::msg(this, tr("No data found for the selected title."));
    } else {
        //FTCommon::msg(this, tr("Data loaded successfully."));
    }
}

void Compare::LoadTitles()
{
    QFile file("CompareData.xml");
    if (!file.exists()) {
        return;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        FTCommon::msg(this, tr("Failed to open data file."));
        return;
    }

    QString currentText = ui->comboBox->currentText();
    ui->comboBox->clear();
    ui->comboBox->setEditText(currentText);

    QXmlStreamReader reader(&file);
    while (!reader.atEnd()) {
        if (reader.isStartElement() && reader.name() == "Entry") {
            QString title = reader.attributes().value("title").toString();
            if (!title.isEmpty() && ui->comboBox->findText(title) == -1) {
                ui->comboBox->addItem(title);
            }
        }
        reader.readNext();
    }

    file.close();

    if (ui->comboBox->count() > 0) {
        ui->comboBox->setCurrentIndex(0);
    }
}

void Compare::TransToLeft()
{
    QVector<TransTask> tasks;
    QModelIndexList indexList = ui->tableWidget->selectionModel()->selectedRows();

    for (int i = 0; i < indexList.size(); ++i) {
        const QTableWidgetItem* itemF = ui->tableWidget->item(indexList[i].row(), 1);
        const QTableWidgetItem* itemT = ui->tableWidget->item(indexList[i].row(), 2);
        TransTask task;
        task.isUpload = false;
        task.localId = GlobalData::Ins()->GetLocalID();
        task.localPath = itemF->text();
        task.remoteId = GlobalData::Ins()->GetRemoteID();
        task.remotePath = itemT->text();
        tasks.push_back(task);
    }

    emit sigTasks(tasks);
}

void Compare::TransToRight()
{
    QVector<TransTask> tasks;
    QModelIndexList indexList = ui->tableWidget->selectionModel()->selectedRows();

    for (int i = 0; i < indexList.size(); ++i) {
        const QTableWidgetItem* itemF = ui->tableWidget->item(indexList[i].row(), 1);
        const QTableWidgetItem* itemT = ui->tableWidget->item(indexList[i].row(), 2);
        TransTask task;
        task.isUpload = true;
        task.localId = GlobalData::Ins()->GetLocalID();
        task.localPath = itemF->text();
        task.remoteId = GlobalData::Ins()->GetRemoteID();
        task.remotePath = itemT->text();
        tasks.push_back(task);
    }                                   

    emit sigTasks(tasks);
}
