#include "CompareControl.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "Form/Loading.h"
#include "GuiUtil/Public.h"
#include "ui_CompareControl.h"

Compare::Compare(QWidget* parent) : QWidget(parent), ui(new Ui::Compare)
{
    ui->setupUi(this);
    InitControl();
    InitMenu();
}

Compare::~Compare()
{
    delete ui;
}

void Compare::InitMenu()
{
    menu_ = new QMenu(ui->tableWidget);
    menu_->addAction(tr("尝试访问本地路径"), this, [this]() {
        auto selected = ui->tableWidget->selectedItems();
        if (selected.size() != 3) {
            return;
        }
        auto item = selected[1];
        auto path = item->text();
        emit sigTryVisit(true, path);
    });
    menu_->addAction(tr("尝试访问远程路径"), this, [this]() {
        auto selected = ui->tableWidget->selectedItems();
        if (selected.size() != 3) {
            return;
        }
        auto item = selected[2];
        auto path = item->text();
        emit sigTryVisit(false, path);
    });
    menu_->addAction(tr("添加新行"), this, [this]() {
        int cnt = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(cnt);
        auto item1 = new QTableWidgetItem("");
        auto item2 = new QTableWidgetItem("");
        auto item3 = new QTableWidgetItem("");
        ui->tableWidget->setItem(cnt, 0, item1);
        ui->tableWidget->setItem(cnt, 1, item2);
        ui->tableWidget->setItem(cnt, 2, item3);
    });
    menu_->addAction(tr("删除"), this, [this]() { deleteSelectedRows(); });
    menu_->addAction(tr("尝试打开本地路径"), this, [this]() {
        auto selected = ui->tableWidget->selectedItems();
        if (selected.size() != 3) {
            return;
        }
        auto item = selected[1];
        auto path = item->text();
        QDir dir(path);
        if (!dir.exists()) {
            FTCommon::msg(this, tr("本地路径不存在。"));
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    menu_->addSeparator();
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { menu_->exec(QCursor::pos()); });
}

void Compare::InitControl()
{
    InitTabWidget();

    connect(ui->btnSave, &QPushButton::clicked, this, &Compare::Save);
    connect(ui->btnLoad, &QPushButton::clicked, this, &Compare::Load);
    connect(ui->btnLeft, &QPushButton::clicked, this, &Compare::TransToLeft);
    connect(ui->btnRight, &QPushButton::clicked, this, &Compare::TransToRight);

    // 测试代码
    connect(ui->btnReplace, &QPushButton::clicked, this, [this]() {
        // auto* loading = new LoadingDialog(this);
        // loading->exec();
    });

    LoadTitles();
}

void Compare::InitTabWidget()
{
    QStringList headers;
    headers << tr("文件") << tr("本地目录") << tr("远端目录");
    ui->tableWidget->setColumnCount(headers.size());
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->comboBox->setEditable(true);
    // ui->tableWidget->setColumnWidth(0, 50);
    ui->tableWidget->setColumnWidth(0, 400);
    /// ui->tableWidget->setColumnWidth(1, 300);
    ui->tableWidget->setColumnWidth(2, 300);

    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    // ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

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
        FTCommon::msg(this, tr("请输入标题"));
        return;
    }

    QFile file("CompareData.xml");
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        FTCommon::msg(this, tr("打开数据文件失败。"));
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
                    if (reader.isStartElement() && reader.name() == "FileName") {
                        paths << reader.readElementText();
                    }
                    if (reader.isStartElement() && reader.name() == "LocalDir") {
                        paths << reader.readElementText();
                    }
                    if (reader.isStartElement() && reader.name() == "RemoteDir") {
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
        QTableWidgetItem* fileName = ui->tableWidget->item(row, 0);
        QTableWidgetItem* localItem = ui->tableWidget->item(row, 1);
        QTableWidgetItem* remoteItem = ui->tableWidget->item(row, 2);
        paths << (fileName ? fileName->text() : "");
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
        for (int i = 0; i < pathList.size(); i += 3) {
            writer.writeTextElement("FileName", pathList[i]);
            writer.writeTextElement("LocalDir", pathList[i + 1]);
            writer.writeTextElement("RemoteDir", pathList[i + 2]);
        }

        writer.writeEndElement();
    }

    writer.writeEndElement();
    writer.writeEndDocument();
    file.close();

    if (ui->comboBox->findText(titleKey) == -1) {
        ui->comboBox->addItem(titleKey);
    }

    FTCommon::msg(this, tr("数据保存成功。"));
}

void Compare::Load()
{
    auto titleKey = ui->comboBox->currentText().trimmed();
    if (titleKey.isEmpty()) {
        FTCommon::msg(this, tr("请选择或输入标题。"));
        return;
    }

    QFile file("CompareData.xml");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        FTCommon::msg(this, tr("打开数据文件失败。"));
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
                    if (reader.isStartElement() && reader.name() == "FileName") {
                        paths << reader.readElementText();
                    }
                    if (reader.isStartElement() && reader.name() == "LocalDir") {
                        paths << reader.readElementText();
                    }
                    if (reader.isStartElement() && reader.name() == "RemoteDir") {
                        paths << reader.readElementText();
                    }
                }
                for (int i = 0; i < paths.size(); i += 3) {
                    int row = ui->tableWidget->rowCount();
                    ui->tableWidget->insertRow(row);

                    QTableWidgetItem* fileName = new QTableWidgetItem(paths[i]);
                    QTableWidgetItem* localItem = new QTableWidgetItem(paths[i + 1]);
                    QTableWidgetItem* remoteItem = new QTableWidgetItem(paths[i + 2]);

                    ui->tableWidget->setItem(row, 0, fileName);
                    ui->tableWidget->setItem(row, 1, localItem);
                    ui->tableWidget->setItem(row, 2, remoteItem);
                }
            }
        }
        reader.readNext();
    }

    file.close();
    if (!found) {
        FTCommon::msg(this, tr("没有找到标题对应的数据。"));
    } else {
        // FTCommon::msg(this, tr("Data loaded successfully."));
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
        const QTableWidgetItem* itemF = ui->tableWidget->item(indexList[i].row(), 2);
        const QTableWidgetItem* itemT = ui->tableWidget->item(indexList[i].row(), 1);
        TransTask task;
        task.taskUUID = Util::UUID();
        task.isUpload = false;
        task.localId = GlobalData::Ins()->GetLocalID();
        task.localPath = itemT->text();
        task.remoteId = GlobalData::Ins()->GetRemoteID();
        task.remotePath = Util::Join(itemF->text(), ui->tableWidget->item(indexList[i].row(), 0)->text());
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
        task.taskUUID = Util::UUID();
        task.isUpload = true;
        task.localId = GlobalData::Ins()->GetLocalID();
        task.localPath = Util::Join(itemF->text(), ui->tableWidget->item(indexList[i].row(), 0)->text());
        task.remoteId = GlobalData::Ins()->GetRemoteID();
        task.remotePath = itemT->text();
        tasks.push_back(task);
    }

    emit sigTasks(tasks);
}

void Compare::deleteSelectedRows()
{
    auto r = FTCommon::affirm(this, tr("确认"), tr("删除选中的行？"));
    if (!r) {
        return;
    }
    QList<int> rowsToDelete;
    for (QTableWidgetItem* item : ui->tableWidget->selectedItems()) {
        int row = item->row();
        if (!rowsToDelete.contains(row)) {
            rowsToDelete.append(row);
        }
    }
    std::sort(rowsToDelete.begin(), rowsToDelete.end(), std::greater<int>());
    for (int row : rowsToDelete) {
        ui->tableWidget->removeRow(row);
    }
}