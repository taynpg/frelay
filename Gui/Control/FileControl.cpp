#include "FileControl.h"

#include <LocalFile.h>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <RemoteFile.h>

#include "ui_FileControl.h"

FileManager::FileManager(QWidget* parent) : QWidget(parent), ui(new Ui::FileManager)
{
    ui->setupUi(this);
    InitControl();
}

FileManager::~FileManager()
{
    delete ui;
}

void FileManager::SetModeStr(const QString& modeStr, int type, ClientCore* clientCore)
{
    ui->lbMode->setText(modeStr);
    if (type == 0) {
        fileHelper_ = std::make_shared<LocalFile>();
        fileHelper_->registerPathCall([this](const QString& path) { ShowPath(path); });
        fileHelper_->registerFileCall([this](const DirFileInfoVec& info) { ShowFile(info); });
    } else {
        auto remotePtr = std::make_shared<RemoteFile>();
        remotePtr->registerPathCall([this](const QString& path) { ShowPath(path); });
        remotePtr->registerFileCall([this](const DirFileInfoVec& info) { ShowFile(info); });
        remotePtr->setClientCore(clientCore);
        fileHelper_ = remotePtr;
    }
}

void FileManager::InitControl()
{
    QStringList headers;
    headers << tr("") << tr("FileName") << tr("ModifyTime") << tr("Type") << tr("Size");
    ui->tableWidget->setColumnCount(headers.size());
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->comboBox->setEditable(true);
    // ui->tableWidget->setColumnWidth(0, 50);
    ui->tableWidget->setColumnWidth(1, 300);
    ui->tableWidget->setColumnWidth(2, 150);
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

    connect(ui->btnHome, &QPushButton::clicked, this, &FileManager::evtHome);
    connect(ui->btnVisit, &QPushButton::clicked, this, &FileManager::evtFile);
    connect(ui->tableWidget, &QTableWidget::cellDoubleClicked, this, &FileManager::doubleClick);
    connect(ui->btnUp, &QPushButton::clicked, this, &FileManager::evtUp);
}

void FileManager::InitMenu(bool remote)
{
    if (remote) {
        auto acDown = new QAction(tr("Download"));
    }
}

void FileManager::ShowPath(const QString& path)
{
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
    QAbstractItemModel* const mdl = ui->tableWidget->model();
    mdl->removeRows(0, mdl->rowCount());
    ui->tableWidget->setRowCount(info.vec.size());

    for (int i = 0; i < info.vec.size(); ++i) {
        const DirFileInfo& fileInfo = info.vec[i];

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
    ui->tableWidget->resizeColumnToContents(0);
    curRoot_ = info.root;
    ShowPath(curRoot_);
}

void FileManager::evtHome()
{
    auto r = fileHelper_->GetHome();
    auto curPath = ui->comboBox->currentText();
    curRoot_ = curPath;
    qDebug() << QString(tr("%1 get home ret:%2").arg(__FUNCTION__).arg(r));
}

void FileManager::evtFile()
{
    auto curPath = ui->comboBox->currentText();
    auto r = fileHelper_->GetDirFile(curPath);
    curRoot_ = curPath;
    qDebug() << QString(tr("%1 get files ret:%2").arg(__FUNCTION__).arg(r));
}

void FileManager::evtUp()
{
    QString path(curRoot_);
    QDir dir(path);
    path = QDir::cleanPath(dir.absolutePath() + "/..");
    if (path.isEmpty()) {
        return;
    }
    auto r = fileHelper_->GetDirFile(path);
    if (r) {
        curRoot_ = path;
        ShowPath(curRoot_);
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

    QDir dir(curRoot_);
    QString np = dir.filePath(item->text());
    fileHelper_->GetDirFile(np);
}
