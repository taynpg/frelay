#include "FileControl.h"

#include <LocalFile.h>
#include <QClipboard>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QScrollBar>
#include <QTableWidgetItem>
#include <RemoteFile.h>

#include "Form/FileInfoForm.h"
#include "Form/Loading.h"
#include "GuiUtil/Public.h"
#include "ui_FileControl.h"

FileManager::FileManager(QWidget* parent) : QWidget(parent), ui(new Ui::FileManager)
{
    ui->setupUi(this);
    InitControl();
    InitMenu();
}

FileManager::~FileManager()
{
    delete ui;
}

void FileManager::SetModeStr(const QString& modeStr, int type, ClientCore* clientCore)
{
    cliCore_ = clientCore;

    ui->lbMode->setText(modeStr);
    if (type == 0) {
        isRemote_ = false;
        fileHelper_ = std::make_shared<LocalFile>();
    } else {
        auto remotePtr = std::make_shared<RemoteFile>();
        remotePtr->setClientCore(clientCore);
        isRemote_ = true;
        ui->tableWidget->setIsRemote(true);
        fileHelper_ = remotePtr;
        connect(cliCore_, &ClientCore::sigDisconnect, this, [this]() { ui->tableWidget->setRowCount(0); });
        connect(cliCore_, &ClientCore::sigOffline, this, [this]() { ui->tableWidget->setRowCount(0); });
    }

    ui->tableWidget->setOwnIDCall([this]() { return cliCore_->GetOwnID(); });
    ui->tableWidget->setRemoteIDCall([this]() { return cliCore_->GetRemoteID(); });

    connect(fileHelper_.get(), &DirFileHelper::sigHome, this, &FileManager::ShowPath);
    connect(fileHelper_.get(), &DirFileHelper::sigDirFile, this, &FileManager::ShowFile);

    if (type == 0) {
        evtHome();
        evtFile();
    }

    if (isRemote_) {
        menu_->addAction(tr("下载"), this, &FileManager::UpDown);
    } else {
        menu_->addAction(tr("上传"), this, &FileManager::UpDown);
    }
}

void FileManager::InitControl()
{
    QStringList headers;
    headers << tr("") << tr("文件名称") << tr("最后修改时间") << tr("类型") << tr("大小");
    ui->tableWidget->setColumnCount(headers.size());
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->comboBox->setEditable(true);
    // ui->tableWidget->setColumnWidth(0, 50);
    ui->tableWidget->setColumnWidth(1, 300);
    ui->tableWidget->setColumnWidth(2, 170);
    ui->tableWidget->setColumnWidth(3, 70);
    ui->tableWidget->setColumnWidth(4, 90);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    // ui->tableWidget->setStyleSheet("QTableWidget::item:hover { background-color: transparent; }");

    ui->tableWidget->setDragEnabled(true);
    ui->tableWidget->viewport()->setAcceptDrops(true);
    ui->tableWidget->setDropIndicatorShown(true);
    ui->tableWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->tableWidget->setDragDropMode(QAbstractItemView::DragDrop);

    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->btnHome, &QPushButton::clicked, this, &FileManager::evtHome);
    connect(ui->btnVisit, &QPushButton::clicked, this, &FileManager::evtFile);
    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &FileManager::doubleClick);
    connect(ui->btnUp, &QPushButton::clicked, this, &FileManager::evtUp);
    connect(ui->tableWidget, &CustomTableWidget::sigTasks, this,
            [this](const QVector<TransTask>& tasks) { emit sigSendTasks(tasks); });

    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested, this,
            [this](const QPoint& pos) { menu_->exec(QCursor::pos()); });

    connect(ui->tableWidget->horizontalHeader(), &QHeaderView::sectionClicked, this, &FileManager::HeaderClicked);

    auto* line = ui->comboBox->lineEdit();
    connect(line, &QLineEdit::returnPressed, this, [this]() { ui->btnVisit->click(); });
    connect(ui->tableWidget->verticalScrollBar(), &QScrollBar::actionTriggered, this, [this]() { userScrol_ = true; });
}

void FileManager::InitMenu()
{
    menu_ = new QMenu(ui->tableWidget);
    menu_->addAction(tr("过滤器"), this, &FileManager::ShowFilterForm);
    menu_->addAction(tr("复制文件路径"), this, &FileManager::CopyFullPath);
    menu_->addAction(tr("属性"), this, &FileManager::ShowProperties);
    menu_->addAction(tr("重命名"), this, &FileManager::OperRename);
    menu_->addAction(tr("删除"), this, &FileManager::OperDelete);
    menu_->addAction(tr("新建文件夹"), this, &FileManager::OperNewFolder);
    menu_->addSeparator();
}

void FileManager::ShowPath(const QString& path, const QVector<QString>& drivers)
{
    QMutexLocker locker(&cbMut_);

    int existingIndex = ui->comboBox->findText(path);
    if (existingIndex != -1) {
        ui->comboBox->removeItem(existingIndex);
    } else if (ui->comboBox->count() >= 20) {
        ui->comboBox->removeItem(ui->comboBox->count() - 1);
    }
    for (const auto& driver : drivers) {
        if (ui->comboBox->findText(driver) == -1) {
            ui->comboBox->insertItem(0, driver);
        }
    }
    if (ui->comboBox->findText(path) == -1) {
        ui->comboBox->insertItem(0, path);
    }
    ui->comboBox->setCurrentIndex(0);
    drivers_ = drivers;
}

void FileManager::ShowFile(const DirFileInfoVec& info)
{
    QMutexLocker locker(&tbMut_);
    currentInfo_ = info;
    SortFileInfo(SortMethod::SMD_BY_TYPE_DESC);
    GenFilter();
    curSelectTypes_.clear();
    currentShowInfo_ = currentInfo_;
    RefreshTab();
    ui->tableWidget->resizeColumnToContents(0);
    SetRoot(currentInfo_.root);
    ShowPath(GetRoot(), drivers_);
}

void FileManager::SetRoot(const QString& path)
{
    if (isRemote_) {
        GlobalData::Ins()->SetRemoteRoot(path);
    } else {
        GlobalData::Ins()->SetLocalRoot(path);
    }
}

void FileManager::SortFileInfo(SortMethod method)
{
    auto& vec = currentInfo_.vec;

    switch (method) {
    case SortMethod::SMD_BY_NAME_ASC:
        std::sort(vec.begin(), vec.end(), [](const DirFileInfo& a, const DirFileInfo& b) {
            return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
        });
        break;

    case SortMethod::SMD_BY_NAME_DESC:
        std::sort(vec.begin(), vec.end(), [](const DirFileInfo& a, const DirFileInfo& b) {
            return QString::compare(a.name, b.name, Qt::CaseInsensitive) > 0;
        });
        break;

    case SortMethod::SMD_BY_TIME_ASC:
        std::sort(vec.begin(), vec.end(),
                  [](const DirFileInfo& a, const DirFileInfo& b) { return a.lastModified < b.lastModified; });
        break;

    case SortMethod::SMD_BY_TIME_DESC:
        std::sort(vec.begin(), vec.end(),
                  [](const DirFileInfo& a, const DirFileInfo& b) { return a.lastModified > b.lastModified; });
        break;

    case SortMethod::SMD_BY_TYPE_ASC:
        std::sort(vec.begin(), vec.end(), [](const DirFileInfo& a, const DirFileInfo& b) {
            if (a.type == b.type) {
                return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
            }
            return a.type < b.type;
        });
        break;

    case SortMethod::SMD_BY_TYPE_DESC:
        std::sort(vec.begin(), vec.end(), [](const DirFileInfo& a, const DirFileInfo& b) {
            if (a.type == b.type) {
                return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
            }
            return a.type > b.type;
        });
        break;

    case SortMethod::SMD_BY_SIZE_ASC:
        std::sort(vec.begin(), vec.end(), [](const DirFileInfo& a, const DirFileInfo& b) {
            if (a.type == b.type) {
                return a.size < b.size;
            }
            return a.type == FileType::Dir && b.type != FileType::Dir;
        });
        break;

    case SortMethod::SMD_BY_SIZE_DESC:
        std::sort(vec.begin(), vec.end(), [](const DirFileInfo& a, const DirFileInfo& b) {
            if (a.type == b.type) {
                return a.size > b.size;
            }
            return a.type == FileType::Dir && b.type != FileType::Dir;
        });
        break;

    default:
        std::sort(vec.begin(), vec.end(), [](const DirFileInfo& a, const DirFileInfo& b) {
            return QString::compare(a.name, b.name, Qt::CaseInsensitive) < 0;
        });
        break;
    }
}

void FileManager::ShowFileItem(const DirFileInfo& f, int i)
{
    static QIcon dirIcon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    static QIcon fileIcon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);
    // ***********************************************************************************
    auto* iconItem = new QTableWidgetItem("");
    iconItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    iconItem->setFlags(iconItem->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(i, 0, iconItem);

    // ***********************************************************************************
    auto* fileItem = new QTableWidgetItem(f.name);
    fileItem->setFlags(fileItem->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(i, 1, fileItem);
    QDateTime modifyTime = QDateTime::fromMSecsSinceEpoch(f.lastModified);

    // ***********************************************************************************
    QString timeStr = modifyTime.toString("yyyy-MM-dd hh:mm:ss");
    auto* timeItem = new QTableWidgetItem(timeStr);
    timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(i, 2, timeItem);

    // ***********************************************************************************
    QString typeStr;
    switch (f.type) {
    case File:
        typeStr = "File";
        iconItem->setIcon(fileIcon);
        break;
    case Dir:
        typeStr = "Dir";
        iconItem->setIcon(dirIcon);
        break;
    case Link:
        typeStr = "Link";
        break;
    case Other:
        typeStr = "Other";
        break;
    default:
        typeStr = "Unknown";
        break;
    }

    // ***********************************************************************************
    auto* typeItem = new QTableWidgetItem(typeStr);
    typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(i, 3, typeItem);

    // ***********************************************************************************
    QString sizeStr;
    if (f.size < 1024) {
        sizeStr = QString::number(f.size) + " B";
    } else if (f.size < 1024 * 1024) {
        sizeStr = QString::number(f.size / 1024.0, 'f', 2) + " KB";
    } else {
        sizeStr = QString::number(f.size / (1024.0 * 1024.0), 'f', 2) + " MB";
    }
    QTableWidgetItem* item = nullptr;
    if (f.type == File) {
        item = new QTableWidgetItem(sizeStr);
    } else {
        item = new QTableWidgetItem("");
    }
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(i, 4, item);
}

void FileManager::RefreshTab()
{
    userScrol_ = false;
    ui->tableWidget->clearContents();
    ui->tableWidget->setRowCount(0);

    for (int i = 0; i < currentShowInfo_.vec.size(); ++i) {

        ui->tableWidget->insertRow(ui->tableWidget->rowCount());
        const DirFileInfo& fileInfo = currentShowInfo_.vec[i];
        ShowFileItem(fileInfo, i);
        if (i != 0 && i % 30 == 0) {
            QGuiApplication::processEvents();
        }
    }

    if (!userScrol_) {
        ui->tableWidget->scrollToTop();
    }
}

void FileManager::HeaderClicked(int column)
{
    SortMethod curMed{};
    switch (column) {
    case 1:
        if (sortMedRecord_.count(column)) {
            curMed = sortMedRecord_[column];
        }
        curMed = (curMed == SortMethod::SMD_BY_NAME_ASC ? SortMethod::SMD_BY_NAME_DESC : SortMethod::SMD_BY_NAME_ASC);
        SortFileInfo(curMed);
        sortMedRecord_[column] = curMed;
        break;
    case 2:
        if (sortMedRecord_.count(column)) {
            curMed = sortMedRecord_[column];
        }
        curMed = (curMed == SortMethod::SMD_BY_TIME_ASC ? SortMethod::SMD_BY_TIME_DESC : SortMethod::SMD_BY_TIME_ASC);
        SortFileInfo(curMed);
        sortMedRecord_[column] = curMed;
        break;
    case 3:
        if (sortMedRecord_.count(column)) {
            curMed = sortMedRecord_[column];
        }
        curMed = (curMed == SortMethod::SMD_BY_TYPE_ASC ? SortMethod::SMD_BY_TYPE_DESC : SortMethod::SMD_BY_TYPE_ASC);
        SortFileInfo(curMed);
        sortMedRecord_[column] = curMed;
        break;
    case 4:
        if (sortMedRecord_.count(column)) {
            curMed = sortMedRecord_[column];
        }
        curMed = (curMed == SortMethod::SMD_BY_SIZE_ASC ? SortMethod::SMD_BY_SIZE_DESC : SortMethod::SMD_BY_SIZE_ASC);
        SortFileInfo(curMed);
        sortMedRecord_[column] = curMed;
        break;
    default:
        return;
    }
    QMetaObject::invokeMethod(this, "RefreshTab", Qt::QueuedConnection);
}

void FileManager::SetUiCurrentPath(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }
    ui->comboBox->addItem(path);
    ui->comboBox->setCurrentText(path);
}

void FileManager::FilterFile(const QVector<QString>& selectedTypes)
{
    if (selectedTypes.contains("*")) {
        currentShowInfo_.vec = currentInfo_.vec;
        QMetaObject::invokeMethod(this, "RefreshTab", Qt::QueuedConnection);
        return;
    }
    currentShowInfo_.vec.clear();
    for (const auto& d : currentInfo_.vec) {
        if (d.type == File) {
            auto ext = d.fullPath.lastIndexOf('.');
            if (ext != -1) {
                QString extStr = d.fullPath.mid(ext + 1).toLower();
                if (selectedTypes.contains(extStr)) {
                    currentShowInfo_.vec.push_back(d);
                }
            }
        }
    }
    QMetaObject::invokeMethod(this, "RefreshTab", Qt::QueuedConnection);
}

void FileManager::GenFilter()
{
    fileTypes_.clear();
    for (const auto& fileInfo : currentInfo_.vec) {
        if (fileInfo.type == File) {
            auto ext = fileInfo.fullPath.lastIndexOf('.');
            if (ext != -1) {
                QString extStr = fileInfo.fullPath.mid(ext + 1).toLower();
                fileTypes_.insert(extStr);
            }
        }
    }
}

void FileManager::ShowFilterForm()
{
    QDialog dialog(this);
    dialog.setWindowTitle("筛选文件类型");
    dialog.resize(400, 300);
    QListWidget listWidget(&dialog);

    listWidget.setSelectionMode(QAbstractItemView::NoSelection);
    QListWidgetItem* allItem = new QListWidgetItem("*(ALL)");
    allItem->setData(Qt::UserRole, "*");
    allItem->setCheckState(curSelectTypes_.contains("*") ? Qt::Checked : Qt::Unchecked);
    listWidget.addItem(allItem);

    for (const QString& ext : fileTypes_) {
        QListWidgetItem* item = new QListWidgetItem(ext);
        item->setData(Qt::UserRole, ext);
        item->setCheckState(curSelectTypes_.contains(ext) ? Qt::Checked : Qt::Unchecked);
        listWidget.addItem(item);
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
        FilterFile(selectedTypes);
        curSelectTypes_.clear();
        for (int i = 0; i < selectedTypes.count(); ++i) {
            curSelectTypes_.insert(selectedTypes[i]);
        }
    }
}

void FileManager::CopyFullPath()
{
    int row = ui->tableWidget->currentRow();
    if (row < 0) {
        return;
    }
    QClipboard* clip = QApplication::clipboard();

    bool found = false;
    QString key = ui->tableWidget->item(row, 1)->text();
    for (const auto& d : currentInfo_.vec) {
        if (d.name == key) {
            clip->setText(d.fullPath);
            found = true;
            return;
        }
    }
    if (!found) {
        FTCommon::msg(this, QString(tr("%1 not found.")).arg(key));
    }
}

void FileManager::ShowProperties()
{
    int row = ui->tableWidget->currentRow();
    if (row < 0) {
        return;
    }
    auto* info = new FileInfo(this);

    info->baseName_ = ui->tableWidget->item(row, 1)->text();
    info->dirName_ = isRemote_ ? GlobalData::Ins()->GetRemoteRoot() : GlobalData::Ins()->GetLocalRoot();
    info->fileTime_ = ui->tableWidget->item(row, 2)->text();
    info->fileType_ = ui->tableWidget->item(row, 3)->text();
    info->fileSize_ = ui->tableWidget->item(row, 4)->text();
    info->setWindowTitle(isRemote_ ? tr("远程文件属性") : tr("本地文件属性"));
    info->exec();
}

void FileManager::UpDown()
{
    auto datas = ui->tableWidget->selectedItems();
    if (datas.isEmpty()) {
        return;
    }
    if (datas.size() % 5 != 0) {
        QMessageBox::information(this, tr("提示"), tr("请选择完整的行。"));
        return;
    }
    QVector<TransTask> tasks;
    for (int i = 0; i < (datas.size() / 5); ++i) {
        if (datas[i * 5 + 3]->text() != "File") {
            qDebug() << QString(tr("暂不支持传输文件夹：%1")).arg(datas[i * 5 + 3]->text());
            continue;
        }
        /*
            要注意这一块的逻辑，本软件的所讲的【上传】【下载】都是针对本地。
            这里的任务拼接和 DropEvent 有所不同，
            DropEvent 是接收方负责拼接任务，但是这里是发送方拼接任务。

            所以这里的拼接逻辑需要注意。
        */
        TransTask task;
        task.taskUUID = Util::UUID();
        task.isUpload = !isRemote_;
        task.localId = cliCore_->GetOwnID();
        task.remoteId = cliCore_->GetRemoteID();
        if (isRemote_) {
            task.remotePath = Util::Join(GlobalData::Ins()->GetRemoteRoot(), datas[i * 5 + 1]->text());
            task.localPath = GlobalData::Ins()->GetLocalRoot();
        } else {
            task.remotePath = GlobalData::Ins()->GetRemoteRoot();
            task.localPath = Util::Join(GlobalData::Ins()->GetLocalRoot(), datas[i * 5 + 1]->text());
        }
        tasks.push_back(task);
    }
    if (tasks.isEmpty()) {
        return;
    }
    emit sigSendTasks(tasks);
}

void FileManager::OperNewFolder()
{
    QInputDialog dialog(this);
    dialog.setWindowTitle("输入");
    dialog.setLabelText("要新建的文件夹名称:");
    dialog.setOkButtonText("确定");
    dialog.setCancelButtonText("取消");
    auto size = dialog.minimumSizeHint();
    size.setWidth(size.width() + 200);
    dialog.setFixedSize(size);

    QString text;
    if (dialog.exec() == QDialog::Accepted) {
        text = dialog.textValue().trimmed();
        if (text.isEmpty()) {
            return;
        }
    } else {
        return;
    }

    auto folder = Util::Join(GetRoot(), text);
    if (!isRemote_) {
        QString ret = Util::NewDir(folder);
        if (ret.isEmpty()) {
            QMessageBox::information(this, tr("提示"), tr("创建%1成功").arg(folder));
            DirFileInfo nf;
            nf.size = 0;
            nf.type = Dir;
            nf.fullPath = folder;
            nf.name = text;
            nf.lastModified = QDateTime::currentDateTime().toMSecsSinceEpoch();
            ui->tableWidget->insertRow(0);
            ShowFileItem(nf, 0);
        } else {
            QMessageBox::information(this, tr("提示"), ret);
        }
        return;
    }

    WaitOper oper(this);
    oper.SetClient(cliCore_);
    oper.SetType(STRMSG_AC_NEW_DIR, STRMSG_AC_ANSWER_NEW_DIR);
    oper.SetPath(folder, "", "");

    LoadingDialog checking(this);
    checking.setTipsText("正在新建...");
    connect(&oper, &WaitOper::sigCheckOver, &checking, &LoadingDialog::cancelBtnClicked);
    connect(cliCore_, &ClientCore::sigMsgAnswer, &oper, &WaitOper::recvFrame);

    oper.start();
    checking.exec();

    std::shared_ptr<void> recv(nullptr, [&oper](void*) { oper.wait(); });

    if (checking.isUserCancel()) {
        oper.interrupCheck();
        return;
    }

    // 检查结果
    auto msg = oper.GetMsg();
    if (msg.msg == STR_NONE || !msg.msg.isEmpty()) {
        QMessageBox::information(this, tr("提示"), QString(tr("新建失败=>%1")).arg(msg.msg));
    } else {
        QMessageBox::information(this, tr("提示"), QString(tr("新建%1成功。")).arg(folder));
        DirFileInfo nf;
        nf.size = 0;
        nf.type = Dir;
        nf.fullPath = folder;
        nf.name = text;
        nf.lastModified = QDateTime::currentDateTime().toMSecsSinceEpoch();
        ui->tableWidget->insertRow(0);
        ShowFileItem(nf, 0);
    }
}

void FileManager::OperDelete()
{
    QList<QTableWidgetItem*> datas;
    if (!CheckSelect(datas)) {
        return;
    }

    // 确认是否删除
    int ret = QMessageBox::question(this, tr("确认删除"), tr("确定要删除%1吗？").arg(datas[1]->text()),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

    if (ret != QMessageBox::Yes) {
        return;
    }

    auto name = Util::Join(GetRoot(), datas[1]->text());
    auto type = datas[3]->text();

    if (!isRemote_) {
        QString ret = Util::Delete(name);
        if (ret.isEmpty()) {
            QMessageBox::information(this, tr("提示"), tr("删除成功"));
            int row = datas[0]->row();
            ui->tableWidget->removeRow(row);
        } else {
            QMessageBox::information(this, tr("提示"), ret);
        }
        return;
    }

    WaitOper oper(this);
    oper.SetClient(cliCore_);
    oper.SetType(STRMSG_AC_DEL_FILEDIR, STRMSG_AC_ANSWER_DEL_FILEDIR);
    oper.SetPath(name, "", "");

    LoadingDialog checking(this);
    checking.setTipsText("正在删除...");
    connect(&oper, &WaitOper::sigCheckOver, &checking, &LoadingDialog::cancelBtnClicked);
    connect(cliCore_, &ClientCore::sigMsgAnswer, &oper, &WaitOper::recvFrame);

    oper.start();
    checking.exec();

    std::shared_ptr<void> recv(nullptr, [&oper](void*) { oper.wait(); });

    if (checking.isUserCancel()) {
        oper.interrupCheck();
        return;
    }

    // 检查结果
    auto msg = oper.GetMsg();
    if (msg.msg == STR_NONE || !msg.msg.isEmpty()) {
        QMessageBox::information(this, tr("提示"), QString(tr("删除失败=>%1")).arg(msg.msg));
    } else {
        QMessageBox::information(this, tr("提示"), QString(tr("删除成功。")));
        ui->tableWidget->removeRow(datas[0]->row());
    }
}

bool FileManager::CheckSelect(QList<QTableWidgetItem*>& ret)
{
    ret = ui->tableWidget->selectedItems();
    if (ret.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请选择一项。"));
        return false;
    }
    if (ret.size() % 5 != 0) {
        QMessageBox::information(this, tr("提示"), tr("请选择单行。"));
        return false;
    }
    return true;
}

void FileManager::OperRename()
{
    QList<QTableWidgetItem*> datas;
    if (!CheckSelect(datas)) {
        return;
    }

    auto curName = datas[1]->text();
    auto curType = datas[3]->text();

    QInputDialog dialog(this);
    dialog.setWindowTitle("输入");
    dialog.setLabelText("请输入新名称:");
    dialog.setOkButtonText("确定");
    dialog.setCancelButtonText("取消");
    auto size = dialog.minimumSizeHint();
    size.setWidth(size.width() + 200);
    dialog.setFixedSize(size);
    dialog.setTextValue(curName);

    QString text;
    if (dialog.exec() == QDialog::Accepted) {
        text = dialog.textValue().trimmed();
        if (text.isEmpty()) {
            return;
        }
    } else {
        return;
    }

    QString oldName = Util::Join(GetRoot(), curName);
    QString newName = Util::Join(GetRoot(), text);

    if (!isRemote_) {
        QString ret = Util::Rename(oldName, newName, curType == STR_DIR);
        if (!ret.isEmpty()) {
            QMessageBox::information(this, tr("提示"), ret);
        } else {
            datas[1]->setText(text);
        }
        return;
    }

    WaitOper oper(this);
    oper.SetClient(cliCore_);
    oper.SetType(STRMSG_AC_RENAME_FILEDIR, STRMSG_AC_ANSWER_RENAME_FILEDIR);
    oper.SetPath(oldName, newName, curType);

    LoadingDialog checking(this);
    checking.setTipsText("正在重命名...");
    connect(&oper, &WaitOper::sigCheckOver, &checking, &LoadingDialog::cancelBtnClicked);
    connect(cliCore_, &ClientCore::sigMsgAnswer, &oper, &WaitOper::recvFrame);

    oper.start();
    checking.exec();

    std::shared_ptr<void> recv(nullptr, [&oper](void*) { oper.wait(); });

    if (checking.isUserCancel()) {
        oper.interrupCheck();
        return;
    }

    // 检查结果
    auto msg = oper.GetMsg();
    if (msg.msg == STR_NONE || !msg.msg.isEmpty()) {
        QMessageBox::information(this, tr("提示"), QString(tr("重命名失败=>%1")).arg(msg.msg));
    } else {
        datas[1]->setText(text);
    }
}

QString FileManager::GetRoot()
{
    if (isRemote_) {
        return GlobalData::Ins()->GetRemoteRoot();
    } else {
        return GlobalData::Ins()->GetLocalRoot();
    }
}

void FileManager::evtHome()
{
    auto r = fileHelper_->GetHome();
    auto curPath = ui->comboBox->currentText();
    SetRoot(curPath);
    qDebug() << QString(tr("%1 获取用户目录结果：%2").arg(__FUNCTION__).arg(r));
}

void FileManager::evtFile()
{
    auto curPath = ui->comboBox->currentText();
    auto r = fileHelper_->GetDirFile(curPath);
    SetRoot(curPath);
    qDebug() << QString(tr("%1 获取文件结果：%2").arg(__FUNCTION__).arg(r));
}

void FileManager::evtUp()
{
    QString path(GetRoot());
    QDir dir(path);
    path = QDir::cleanPath(dir.absolutePath() + "/..");
    if (path.isEmpty()) {
        return;
    }
    auto r = fileHelper_->GetDirFile(path);
    if (r) {
        SetRoot(path);
        ShowPath(GetRoot(), drivers_);
    }
}

void FileManager::doubleClick(int row, int column)
{
    Q_UNUSED(column)

    auto* item = ui->tableWidget->item(row, 1);
    if (item == nullptr) {
        return;
    }

    auto type = ui->tableWidget->item(row, 3)->text();
    if (type != STR_DIR) {
        return;
    }

    QDir dir(GetRoot());
    QString np = dir.filePath(item->text());
    fileHelper_->GetDirFile(np);
}

WaitOper::WaitOper(QObject* parent) : WaitThread(parent)
{
}

void WaitOper::run()
{
    isAlreadyInter_ = false;
    infoMsg_.msg = STR_NONE;
    isRun_ = true;
    recvMsg_ = false;

    InfoMsg msg;
    msg.command = sendStrType_;
    msg.fromPath = stra_;
    msg.toPath = strb_;
    msg.type = type_;

    auto f = cli_->GetBuffer<InfoMsg>(msg, FBT_MSGINFO_ASK, cli_->GetRemoteID());
    if (!ClientCore::syncInvoke(cli_, f)) {
        auto errMsg = QString(tr("向%1发送%2请求失败。")).arg(cli_->GetRemoteID()).arg(sendStrType_);
        emit sigCheckOver();
        qCritical() << errMsg;
        return;
    }
    while (isRun_) {
        QThread::msleep(1);
        if (isAlreadyInter_) {
            qInfo() << tr("线程中断文件操作等待......");
            return;
        }
        if (!recvMsg_) {
            continue;
        }
        break;
    }
    isAlreadyInter_ = true;
    emit sigCheckOver();
    auto n = QString(tr("向%1的请求%2处理结束。")).arg(cli_->GetRemoteID()).arg(sendStrType_);
    qInfo() << n;
}

void WaitOper::SetType(const QString& sendType, const QString& ansType)
{
    sendStrType_ = sendType;
    ansStrType_ = ansType;
}

void WaitOper::SetPath(const QString& stra, const QString& strb, const QString& type)
{
    stra_ = stra;
    strb_ = strb;
    type_ = type;
}

InfoMsg WaitOper::GetMsg() const
{
    return infoMsg_;
}

void WaitOper::interrupCheck()
{
    qWarning() << QString(tr("中断请求处理%1......")).arg(sendStrType_);
    WaitThread::interrupCheck();
}

void WaitOper::recvFrame(QSharedPointer<FrameBuffer> frame)
{
    InfoMsg info = infoUnpack<InfoMsg>(frame->data);
    if (info.command == ansStrType_) {
        infoMsg_ = info;
        recvMsg_ = true;
        return;
    }
    auto n = tr("收到未知Oper的回复信息：") + info.command;
    qInfo() << n;
}
