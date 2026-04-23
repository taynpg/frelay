#include "CompareControl.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QMessageBox>
#include <tinyxml2.h>

#include "Form/Loading.h"
#include "GuiUtil/Public.h"
#include "ui_CompareControl.h"

using namespace tinyxml2;

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
    ui->comboBox->setMinimumWidth(150);
    menu_ = new QMenu(ui->tableWidget);
    menu_->addAction(tr("尝试访问本地路径"), this, [this]() {
        auto selected = ui->tableWidget->selectedItems();
        if (selected.size() != 5) {
            return;
        }
        auto item = selected[3];
        auto path = item->text();
        emit sigTryVisit(true, path);
    });
    menu_->addAction(tr("尝试访问远程路径"), this, [this]() {
        auto selected = ui->tableWidget->selectedItems();
        if (selected.size() != 5) {
            return;
        }
        auto item = selected[4];
        auto path = item->text();
        emit sigTryVisit(false, path);
    });
    menu_->addAction(tr("尝试打开本地路径"), this, [this]() {
        auto selected = ui->tableWidget->selectedItems();
        if (selected.size() != 5) {
            return;
        }
        auto item = selected[3];
        auto path = item->text();
        QDir dir(path);
        if (!dir.exists()) {
            FTCommon::msg(this, tr("本地路径不存在。"));
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    });
    menu_->addAction(tr("添加新行"), this, [this]() {
        int cnt = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(cnt);
        auto item1 = new QTableWidgetItem("");
        auto item2 = new QTableWidgetItem("");
        auto item3 = new QTableWidgetItem("");
        auto item4 = new QTableWidgetItem("");
        auto item5 = new QTableWidgetItem("");
        ui->tableWidget->setItem(cnt, 0, item1);
        ui->tableWidget->setItem(cnt, 1, item2);
        ui->tableWidget->setItem(cnt, 2, item3);
        ui->tableWidget->setItem(cnt, 3, item4);
        ui->tableWidget->setItem(cnt, 4, item5);
    });
    menu_->addAction(tr("删除"), this, [this]() { deleteSelectedRows(); });
    menu_->addAction(tr("上传"), this, [this]() { TransToRight(false); });
    menu_->addAction(tr("下载"), this, [this]() { TransToLeft(false); });
    menu_->addSeparator();
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { menu_->exec(QCursor::pos()); });
}

void Compare::InitControl()
{
    InitTabWidget();

    connect(ui->btnSave, &QPushButton::clicked, this, &Compare::Save);
    connect(ui->btnLoad, &QPushButton::clicked, this, &Compare::Load);
    // connect(ui->btnLeft, &QPushButton::clicked, this, &Compare::TransToLeft);
    // connect(ui->btnRight, &QPushButton::clicked, this, &Compare::TransToRight);
    connect(ui->btnTypeDown, &QPushButton::clicked, this, [this]() { FilterFiles(false); });
    connect(ui->btnTypeUpload, &QPushButton::clicked, this, [this]() { FilterFiles(true); });
    connect(ui->btnSearch, &QPushButton::clicked, this, &Compare::Search);
    connect(ui->btnReset, &QPushButton::clicked, this, &Compare::Reset);
    connect(ui->btnDel, &QPushButton::clicked, this, &Compare::DelTitle);

    // 测试代码
    connect(ui->btnReplace, &QPushButton::clicked, this, [this]() {
        // auto* loading = new LoadingDialog(this);
        // loading->exec();
    });

    ui->cbMark->setMinimumWidth(90);
    ui->cbMark->clear();
    ui->cbMark->addItem("ALL");
    ui->cbMark->setCurrentIndex(0);
    connect(ui->cbMark, &QComboBox::currentTextChanged, this, [this]() { FilterMark(); });

    LoadTitles();

    isResource_ = true;
    ui->tableWidget->setIsResource(isResource_);
}

void Compare::InitTabWidget()
{
    QStringList headers;
    headers << tr("文件") << tr("标记") << tr("类型") << tr("本地目录") << tr("远端目录");
    ui->tableWidget->setColumnCount(headers.size());
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->comboBox->setEditable(true);
    // ui->tableWidget->setColumnWidth(0, 50);
    ui->tableWidget->setColumnWidth(0, 260);
    ui->tableWidget->setColumnWidth(1, 100);
    ui->tableWidget->setColumnWidth(2, 100);
    ui->tableWidget->setColumnWidth(3, 200);

    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    // ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    // ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
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

    if (!isResource_) {
        QMessageBox::information(this, tr("提示"), tr("请先重置搜索结果后操作。"));
        return;
    }

    if (ui->tableWidget->rowCount() < 1) {
        if (!FTCommon::affirm(this, tr("确认"), tr("保存的空数据，继续？"))) {
            return;
        }
    }

    // 使用tinyxml2加载或创建XML文档
    XMLDocument doc;

    // 尝试加载现有文件
    if (doc.LoadFile("CompareData.xml") != XML_SUCCESS) {
        // 如果文件不存在或无法读取，创建一个新的文档
        XMLElement* root = doc.NewElement("CompareData");
        doc.InsertFirstChild(root);
    }

    // 获取或创建根元素
    XMLElement* root = doc.RootElement();
    if (!root) {
        root = doc.NewElement("CompareData");
        doc.InsertFirstChild(root);
    }

    // 查找或创建对应的Entry元素
    XMLElement* entryElem = nullptr;
    for (XMLElement* elem = root->FirstChildElement("Entry"); elem != nullptr; elem = elem->NextSiblingElement("Entry")) {
        const char* title = elem->Attribute("title");
        if (title && QString(title) == titleKey) {
            entryElem = elem;
            // 删除现有的Item子元素
            while (XMLElement* child = elem->FirstChildElement("Item")) {
                elem->DeleteChild(child);
            }
            break;
        }
    }

    // 如果没有找到对应的Entry，创建新的
    if (!entryElem) {
        entryElem = doc.NewElement("Entry");
        entryElem->SetAttribute("title", titleKey.toUtf8().constData());
        root->InsertEndChild(entryElem);
    }

    // 获取表格中的数据
    items_.clear();
    for (int row = 0; row < ui->tableWidget->rowCount(); ++row) {
        QTableWidgetItem* fileName = ui->tableWidget->item(row, 0);
        QTableWidgetItem* localItem = ui->tableWidget->item(row, 3);
        QTableWidgetItem* remoteItem = ui->tableWidget->item(row, 4);
        QTableWidgetItem* markItem = ui->tableWidget->item(row, 1);   // 假设第4列是mark
        QTableWidgetItem* typeItem = ui->tableWidget->item(row, 2);   // 假设第5列是type

        auto baseName = (fileName ? fileName->text() : "");
        auto localDir = (localItem ? localItem->text() : "");
        auto remoteDir = (remoteItem ? remoteItem->text() : "");
        auto mark = (markItem ? markItem->text() : "");
        auto type = (typeItem ? typeItem->text() : "");

        CompareItem item;
        item.baseName = baseName;
        item.localDir = localDir;
        item.remoteDir = remoteDir;
        item.mark = mark;
        item.type = type;
        items_.push_back(item);

        // 创建Item元素
        XMLElement* itemElem = doc.NewElement("Item");
        itemElem->SetAttribute("FileName", baseName.toUtf8().constData());
        itemElem->SetAttribute("LocalDir", localDir.toUtf8().constData());
        itemElem->SetAttribute("RemoteDir", remoteDir.toUtf8().constData());
        itemElem->SetAttribute("Mark", mark.toUtf8().constData());
        itemElem->SetAttribute("Type", type.toUtf8().constData());

        entryElem->InsertEndChild(itemElem);
    }

    // 保存到文件
    if (doc.SaveFile("CompareData.xml") != XML_SUCCESS) {
        FTCommon::msg(this, tr("保存数据文件失败。"));
        return;
    }

    // 更新下拉框
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

    // 使用tinyxml2加载XML文档
    XMLDocument doc;
    if (doc.LoadFile("CompareData.xml") != XML_SUCCESS) {
        FTCommon::msg(this, tr("打开数据文件失败。"));
        return;
    }

    XMLElement* root = doc.RootElement();
    if (!root) {
        FTCommon::msg(this, tr("数据文件格式错误。"));
        return;
    }

    items_.clear();
    isResource_ = true;
    ui->tableWidget->setIsResource(isResource_);
    ui->tableWidget->setRowCount(0);

    bool found = false;

    // 查找对应的Entry元素
    for (XMLElement* entryElem = root->FirstChildElement("Entry"); entryElem != nullptr;
         entryElem = entryElem->NextSiblingElement("Entry")) {

        const char* title = entryElem->Attribute("title");
        if (title && QString(title) == titleKey) {
            found = true;

            // 遍历所有的Item子元素
            for (XMLElement* itemElem = entryElem->FirstChildElement("Item"); itemElem != nullptr;
                 itemElem = itemElem->NextSiblingElement("Item")) {

                const char* fileName = itemElem->Attribute("FileName");
                const char* localDir = itemElem->Attribute("LocalDir");
                const char* remoteDir = itemElem->Attribute("RemoteDir");
                const char* mark = itemElem->Attribute("Mark");
                const char* type = itemElem->Attribute("Type");

                if (fileName && localDir && remoteDir) {
                    int row = ui->tableWidget->rowCount();
                    ui->tableWidget->insertRow(row);

                    CompareItem item;
                    item.baseName = QString::fromUtf8(fileName);
                    item.localDir = QString::fromUtf8(localDir);
                    item.remoteDir = QString::fromUtf8(remoteDir);
                    item.mark = mark ? QString::fromUtf8(mark) : "";
                    item.type = type ? QString::fromUtf8(type) : "";
                    items_.push_back(item);

                    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(item.baseName));
                    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(item.localDir));
                    ui->tableWidget->setItem(row, 4, new QTableWidgetItem(item.remoteDir));
                    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(item.mark));
                    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(item.type));
                }
            }
            break;
        }
    }

    if (!found) {
        FTCommon::msg(this, tr("没有找到标题对应的数据。"));
    } else {
        RefreshMark();
    }
}

void Compare::DelTitle()
{
    auto titleKey = ui->comboBox->currentText().trimmed();
    if (titleKey.isEmpty()) {
        FTCommon::msg(this, tr("请选择要删除的标题。"));
        return;
    }

    // 确认对话框
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, tr("确认删除"), tr("确定要删除标题 '%1' 及其所有数据吗？").arg(titleKey),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;   // 用户取消删除
    }

    // 使用tinyxml2加载XML文档
    XMLDocument doc;
    if (doc.LoadFile("CompareData.xml") != XML_SUCCESS) {
        FTCommon::msg(this, tr("打开数据文件失败。"));
        return;
    }

    XMLElement* root = doc.RootElement();
    if (!root) {
        FTCommon::msg(this, tr("数据文件格式错误。"));
        return;
    }

    bool found = false;
    XMLElement* entryToDelete = nullptr;

    // 查找对应的Entry元素
    for (XMLElement* entryElem = root->FirstChildElement("Entry"); entryElem != nullptr;
         entryElem = entryElem->NextSiblingElement("Entry")) {

        const char* title = entryElem->Attribute("title");
        if (title && QString(title) == titleKey) {
            entryToDelete = entryElem;
            found = true;
            break;
        }
    }

    if (!found) {
        FTCommon::msg(this, tr("没有找到标题 '%1' 对应的数据。").arg(titleKey));
        return;
    }

    // 删除找到的Entry元素
    if (entryToDelete) {
        root->DeleteChild(entryToDelete);
    }

    // 保存修改后的XML文件
    if (doc.SaveFile("CompareData.xml") != XML_SUCCESS) {
        FTCommon::msg(this, tr("保存数据文件失败。"));
        return;
    }

    // 重新加载标题到下拉框
    ui->comboBox->clear();
    LoadTitles();

    // 清空表格中的数据
    items_.clear();
    isResource_ = true;
    ui->tableWidget->setIsResource(true);
    ui->tableWidget->setRowCount(0);

    FTCommon::msg(this, tr("标题 '%1' 已删除。").arg(titleKey));
}

void Compare::FilterMark()
{
    if (isAutoChangeFilter_) {
        return;
    }
    auto key = ui->cbMark->currentText();
    if (key.isEmpty()) {
        return;
    }

    key = key.toUpper();
    QVector<CompareItem> result;
    for (const auto& item : TAS_CONST(items_)) {
        auto markName = item.mark.toUpper();
        if (markName == key || key == "ALL") {
            result.push_back(item);
        }
    }
    SetResult(result);

    if (key == "ALL") {
        isResource_ = true;
    } else {
        isResource_ = false;
    }
    ui->tableWidget->setIsResource(isResource_);
}

void Compare::RefreshMark()
{
    isAutoChangeFilter_ = true;
    QStringList re;
    re << "ALL";
    for (const auto& item : TAS_CONST(items_)) {
        auto k = item.mark.toLower();
        if (!k.isEmpty() && !re.contains(k)) {
            re.append(k);
        }
    }
    ui->cbMark->clear();
    ui->cbMark->addItems(re);
    ui->cbMark->setCurrentIndex(0);
    isAutoChangeFilter_ = false;
}

void Compare::LoadTitles()
{
    // 使用tinyxml2加载XML文档
    XMLDocument doc;
    if (doc.LoadFile("CompareData.xml") != XML_SUCCESS) {
        return;   // 文件不存在是正常的
    }

    XMLElement* root = doc.RootElement();
    if (!root) {
        return;
    }

    QString currentText = ui->comboBox->currentText();
    ui->comboBox->clear();
    ui->comboBox->setEditText(currentText);

    // 遍历所有的Entry元素，获取title属性
    for (XMLElement* entryElem = root->FirstChildElement("Entry"); entryElem != nullptr;
         entryElem = entryElem->NextSiblingElement("Entry")) {

        const char* title = entryElem->Attribute("title");
        if (title && !QString(title).isEmpty() && ui->comboBox->findText(QString(title)) == -1) {
            ui->comboBox->addItem(QString(title));
        }
    }

    if (ui->comboBox->count() > 0) {
        ui->comboBox->setCurrentIndex(0);
    }
}

void Compare::Search()
{
    auto key = ui->edSearch->text().trimmed();
    if (key.isEmpty()) {
        return;
    }

    key = key.toUpper();
    QVector<CompareItem> result;
    for (const auto& item : TAS_CONST(items_)) {
        auto baseNameU = item.baseName.toUpper();
        auto localDirU = item.localDir.toUpper();
        auto remoteDirU = item.remoteDir.toUpper();

        if (baseNameU.contains(key) || localDirU.contains(key) || remoteDirU.contains(key)) {
            result.push_back(item);
        }
    }
    SetResult(result);
    isResource_ = false;
    ui->tableWidget->setIsResource(isResource_);
}

void Compare::Reset()
{
    SetResult(items_);
    isResource_ = true;
    ui->tableWidget->setIsResource(isResource_);
}

void Compare::SetResult(const QVector<CompareItem>& items)
{
    ui->tableWidget->setRowCount(0);
    for (const auto& item : items) {
        int row = ui->tableWidget->rowCount();
        ui->tableWidget->insertRow(row);
        ui->tableWidget->setItem(row, 0, new QTableWidgetItem(item.baseName));
        ui->tableWidget->setItem(row, 1, new QTableWidgetItem(item.mark));
        ui->tableWidget->setItem(row, 2, new QTableWidgetItem(item.type));
        ui->tableWidget->setItem(row, 3, new QTableWidgetItem(item.localDir));
        ui->tableWidget->setItem(row, 4, new QTableWidgetItem(item.remoteDir));
    }
}

void Compare::FilterFiles(bool isUpload)
{
    QDialog dialog(this);
    QString title = QString("筛选文件类型(%1)").arg(isUpload ? tr("上传") : tr("下载"));
    dialog.setWindowTitle(title);
    dialog.resize(400, 300);
    QListWidget listWidget(&dialog);

    listWidget.setSelectionMode(QAbstractItemView::NoSelection);
    QListWidgetItem* allItem = new QListWidgetItem("*(ALL)");
    allItem->setData(Qt::UserRole, "*ALL");
    allItem->setCheckState(curSelectTypes_.contains("*") ? Qt::Checked : Qt::Unchecked);
    listWidget.addItem(allItem);

    std::map<QString, int> typeCounts;
    int rows = ui->tableWidget->rowCount();
    for (int i = 0; i < rows; ++i) {
        QString ext = ui->tableWidget->item(i, 0)->text().split(".").last().toLower();
        if (typeCounts.count(ext) < 1) {
            QListWidgetItem* item = new QListWidgetItem(ext);
            item->setData(Qt::UserRole, ext);
            item->setCheckState(curSelectTypes_.contains(ext) ? Qt::Checked : Qt::Unchecked);
            listWidget.addItem(item);
            typeCounts[ext] = 1;
        }
    }

    QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(&buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    QVBoxLayout layout(&dialog);
    layout.addWidget(&listWidget);
    layout.addWidget(&buttons);
    dialog.setLayout(&layout);

    if (dialog.exec() == QDialog::Accepted) {
        QVector<QString> selectedTypes;
        for (int i = 0; i < listWidget.count(); ++i) {
            QListWidgetItem* item = listWidget.item(i);
            if (item->checkState() == Qt::Checked) {
                selectedTypes << item->data(Qt::UserRole).toString();
            }
        }
        curSelectTypes_.clear();
        for (int i = 0; i < selectedTypes.count(); ++i) {
            curSelectTypes_.insert(selectedTypes[i]);
        }
        if (isUpload) {
            TransToRight(true);
        } else {
            TransToLeft(true);
        }
    }
}

void Compare::TransToLeft(bool useSelectTypes)
{
    QVector<TransTask> tasks;

    auto pushTask = [&](const QString& localPath, const QString& remotePath) {
        TransTask task;
        task.taskUUID = Util::UUID();
        task.isUpload = false;
        task.localId = GlobalData::Ins()->GetLocalID();
        task.localPath = localPath;
        task.remoteId = GlobalData::Ins()->GetRemoteID();
        task.remotePath = remotePath;
        tasks.push_back(task);
    };

    if (useSelectTypes) {
        for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
            QString ext = ui->tableWidget->item(i, 0)->text().split(".").last().toLower();
            if (curSelectTypes_.contains(ext) || curSelectTypes_.contains("*ALL")) {
                const QTableWidgetItem* itemF = ui->tableWidget->item(i, 3);
                const QTableWidgetItem* itemT = ui->tableWidget->item(i, 4);
                pushTask(itemT->text(), Util::Join(itemF->text(), ui->tableWidget->item(i, 0)->text()));
            }
        }
        if (tasks.size() > 0) {
            emit sigTasks(tasks);
        }
    } else {
        QModelIndexList indexList = ui->tableWidget->selectionModel()->selectedRows();
        if (indexList.size() < 1) {
            QMessageBox::information(this, tr("提示"), tr("请选择要下载的文件。"));
            return;
        }
        for (int i = 0; i < indexList.size(); ++i) {
            const QTableWidgetItem* itemF = ui->tableWidget->item(indexList[i].row(), 4);
            const QTableWidgetItem* itemT = ui->tableWidget->item(indexList[i].row(), 3);
            pushTask(itemT->text(), Util::Join(itemF->text(), ui->tableWidget->item(indexList[i].row(), 0)->text()));
        }
        if (tasks.size() > 0) {
            emit sigTasks(tasks);
        }
    }
}

void Compare::TransToRight(bool useSelectTypes)
{
    QVector<TransTask> tasks;

    auto pushTask = [&](const QString& localPath, const QString& remotePath) {
        TransTask task;
        task.taskUUID = Util::UUID();
        task.isUpload = true;
        task.localId = GlobalData::Ins()->GetLocalID();
        task.localPath = localPath;
        task.remoteId = GlobalData::Ins()->GetRemoteID();
        task.remotePath = remotePath;
        tasks.push_back(task);
    };

    if (useSelectTypes) {
        for (int i = 0; i < ui->tableWidget->rowCount(); ++i) {
            QString ext = ui->tableWidget->item(i, 0)->text().split(".").last().toLower();
            if (curSelectTypes_.contains(ext) || curSelectTypes_.contains("*ALL")) {
                const QTableWidgetItem* itemF = ui->tableWidget->item(i, 3);
                const QTableWidgetItem* itemT = ui->tableWidget->item(i, 4);
                pushTask(Util::Join(itemF->text(), ui->tableWidget->item(i, 0)->text()), itemT->text());
            }
        }
        if (tasks.size() > 0) {
            emit sigTasks(tasks);
        }
    } else {
        QModelIndexList indexList = ui->tableWidget->selectionModel()->selectedRows();
        if (indexList.size() < 1) {
            QMessageBox::information(this, tr("提示"), tr("请选择要上传的文件。"));
            return;
        }
        for (int i = 0; i < indexList.size(); ++i) {
            const QTableWidgetItem* itemF = ui->tableWidget->item(indexList[i].row(), 3);
            const QTableWidgetItem* itemT = ui->tableWidget->item(indexList[i].row(), 4);
            pushTask(Util::Join(itemF->text(), ui->tableWidget->item(indexList[i].row(), 0)->text()), itemT->text());
        }
        if (tasks.size() > 0) {
            emit sigTasks(tasks);
        }
    }
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
