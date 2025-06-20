#include "cpTableWidget.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

CpTableWidget::CpTableWidget(QWidget* parent) : QTableWidget(parent)
{

}

CpTableWidget::~CpTableWidget()
{

}

void CpTableWidget::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->ignore();
        return;
    }
    QByteArray encoded = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
    QDataStream stream(&encoded, QIODevice::ReadOnly);

    QList<QTableWidgetItem*> draggedItems;
    while (!stream.atEnd()) {
        int row, col;
        QMap<int, QVariant> roleData;
        stream >> row >> col >> roleData;
        if (col != 1) {
            continue;
        }
    }
}

void CpTableWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}


