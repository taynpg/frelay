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
    if (!event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->ignore();
        return;
    }
    QByteArray encoded = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    QVector<TransTask> tasks;
    QList<QTableWidgetItem*> draggedItems;
    while (!stream.atEnd()) {
        int row, col;
        QMap<int, QVariant> roleData;
        stream >> row >> col >> roleData;
        if (col != 1) {
            continue;
        }
        TransTask task;
        task.isUpload = isRemote_;
        task.localId = oidCall_();
        task.remoteId = ridCall_();
        if (isRemote_) {
            task.remotePath = GlobalData::Ins()->GetRemoteRoot();
            task.localPath = Util::Join(GlobalData::Ins()->GetLocalRoot(), roleData[Qt::DisplayRole].toString());
        } else {
            task.localPath = GlobalData::Ins()->GetLocalRoot();
            task.remotePath = Util::Join(GlobalData::Ins()->GetRemoteRoot(), roleData[Qt::DisplayRole].toString());
        }
        tasks.push_back(task);
    }
    emit sigTasks(tasks);
}

void CustomTableWidget::dragEnterEvent(QDragEnterEvent* event)
{
    const QTableWidget* source = qobject_cast<const QTableWidget*>(event->source());
    if (source == this) {
        event->ignore();
        return;
    }
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}