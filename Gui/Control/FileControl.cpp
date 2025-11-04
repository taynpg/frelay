#include "FileControl.h"

#include <LocalFile.h>
#include <QClipboard>
#include <QDateTime>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QHeaderView>
#include <QLineEdit>
#include <QListWidget>
#include <QTableWidgetItem>
#include <RemoteFile.h>

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
}

void FileManager::InitMenu()
{
    menu_ = new QMenu(ui->tableWidget);
    menu_->addAction(tr("Filter"), this, &FileManager::ShowFilterForm);
    menu_->addAction(tr("FullPath"), this, &FileManager::CopyFullPath);
    menu_->addSeparator();
}

void FileManager::ShowPath(const QString& path)
{
    QMutexLocker locker(&cbMut_);
    int existingIndex = ui->comboBox->findText(path);
    if (existingIndex != -1) {
        ui->comboBox->removeItem(existingIndex);
    } else if (ui->comboBox->count() >= 20) {
        ui->comboBox->removeItem(ui->comboBox->count() - 1);
    }
    ui->comboBox->insertItem(0, path);
    ui->comboBox->setCurrentIndex(0);
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
    ShowPath(GetRoot());
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

void FileManager::RefreshTab()
{
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setRowCount(currentShowInfo_.vec.size());
    for (int i = 0; i < currentShowInfo_.vec.size(); ++i) {
        const DirFileInfo& fileInfo = currentShowInfo_.vec[i];

        // ***********************************************************************************
        auto* iconItem = new QTableWidgetItem("");
        iconItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
        iconItem->setFlags(iconItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 0, iconItem);

        // ***********************************************************************************
        auto* fileItem = new QTableWidgetItem(fileInfo.name);
        fileItem->setFlags(fileItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 1, fileItem);
        QDateTime modifyTime = QDateTime::fromMSecsSinceEpoch(fileInfo.lastModified);

        // ***********************************************************************************
        QString timeStr = modifyTime.toString("yyyy-MM-dd hh:mm:ss");
        auto* timeItem = new QTableWidgetItem(timeStr);
        timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 2, timeItem);

        // ***********************************************************************************
        QString typeStr;
        switch (fileInfo.type) {
        case File:
            typeStr = "File";
            iconItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
            break;
        case Dir:
            typeStr = "Dir";
            iconItem->setIcon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
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
        if (fileInfo.size < 1024) {
            sizeStr = QString::number(fileInfo.size) + " B";
        } else if (fileInfo.size < 1024 * 1024) {
            sizeStr = QString::number(fileInfo.size / 1024.0, 'f', 2) + " KB";
        } else {
            sizeStr = QString::number(fileInfo.size / (1024.0 * 1024.0), 'f', 2) + " MB";
        }
        QTableWidgetItem* item = nullptr;
        if (fileInfo.type == File) {
            item = new QTableWidgetItem(sizeStr);
        } else {
            item = new QTableWidgetItem("");
        }
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        ui->tableWidget->setItem(i, 4, item);
    }
    ui->tableWidget->scrollToTop();
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

void FileManager::FilterFile(const QStringList& selectedTypes)
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
    dialog.setWindowTitle("Select file type");
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
        QStringList selectedTypes;
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
    qDebug() << QString(tr("%1 获取家目录结果：%2").arg(__FUNCTION__).arg(r));
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
        ShowPath(GetRoot());
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
    if (type != "Dir") {
        return;
    }

    QDir dir(GetRoot());
    QString np = dir.filePath(item->text());
    fileHelper_->GetDirFile(np);
}
