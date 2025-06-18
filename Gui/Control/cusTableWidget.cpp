#include "cusTableWidget.h"

#include <InfoDirFile.h>
#include <InfoPack.hpp>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include "FileControl.h"

CustomTableWidget::CustomTableWidget(QWidget* parent) : QTableWidget(parent)
{
}

CustomTableWidget::~CustomTableWidget()
{
}

void CustomTableWidget::setIsRemote(bool isRemote)
{
    isRemote_ = isRemote;
}

void CustomTableWidget::setBasePathCall(const std::function<QString()>& call)
{
    basePathCall_ = call;
}

QString FileManager::GetCurRoot()
{
    return curRoot_;
}

void CustomTableWidget::setOwnIDCall(const std::function<QString()>& call)
{
    oidCall_ = call;
}

void CustomTableWidget::setRemoteIDCall(const std::function<QString()>& call)
{
    ridCall_ = call;
}

void CustomTableWidget::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasFormat("application/x-custom-data")) {
        event->ignore();
        return;
    }
    QTableWidget::dropEvent(event);
}

void CustomTableWidget::dragEnterEvent(QDragEnterEvent* event)
{
    const QTableWidget* source = qobject_cast<const QTableWidget*>(event->source());
    if (source == this) {
        event->ignore();
        return;
    }
    if (event->mimeData()->hasFormat("application/x-custom-data")) {
        event->acceptProposedAction();
        auto dirinfo = infoUnpack<DirFileInfoVec>(event->mimeData()->data("application/x-custom-data"));
        QVector<TransTask> tasks;
        // generate task
        for (const auto& df : dirinfo.vec) {
            TransTask task;
            task.isUpload = isRemote_;
            task.localId = oidCall_();
            task.remoteId = ridCall_();
            if (isRemote_) {
                task.remotePath = basePathCall_();
                task.localPath = Util::Join(dirinfo.root, df.name);
            }
            else {
                task.remotePath = Util::Join(dirinfo.root, df.name);
                task.localPath = basePathCall_();
            }
            tasks.push_back(task);
        }
        emit sigTasks(tasks);
    } else {
        event->ignore();
    }
}

void CustomTableWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    if ((event->pos() - startPos_).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;

    DirFileInfoVec v;
    v.root = basePathCall_();
    foreach (QTableWidgetItem* item, selectedItems()) {
        if (item->column() == 1) {
            DirFileInfo df;
            df.name = item->text();
            v.vec.push_back(df);
        }
    }
    mimeData->setData("application/x-custom-data", infoPack<DirFileInfoVec>(v));
    drag->setMimeData(mimeData);
    QPixmap pixmap(100, 50);
    pixmap.fill(Qt::lightGray);
    QPainter painter(&pixmap);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, QString("%1 ITEM").arg(v.vec.size()));
    drag->setPixmap(pixmap);
    drag->exec(Qt::CopyAction | Qt::MoveAction);
}

void CustomTableWidget::mousePressEvent(QMouseEvent* event)
{
    QTableWidget::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        startPos_ = event->pos();
    }
}
