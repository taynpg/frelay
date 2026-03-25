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

void CustomTableWidget::setClientCore(ClientCore* clientCore)
{
    clientCore_ = clientCore;
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

    QStringList parseData;
    QVector<TransTask> tasks;
    while (!stream.atEnd()) {
        int row, col;
        QMap<int, QVariant> roleData;
        stream >> row >> col >> roleData;
        parseData.append(roleData[Qt::DisplayRole].toString());
    }

    QVector<QString> datas;
    for (int i = 0; i < parseData.size(); ++i) {
        datas.append(parseData[i]);
    }

    FileManager::UpDownCommon(datas, 5, tasks, true, clientCore_, isRemote_, this);

    if (tasks.empty()) {
        return;
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