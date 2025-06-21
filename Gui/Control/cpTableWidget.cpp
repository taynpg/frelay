#include "cpTableWidget.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <Util.h>

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

    QPoint pos = event->pos();
    int startRow = rowAt(pos.y());
    int startCol = columnAt(pos.x());
    if (startCol != 1 && startCol != 2) {
        event->ignore();
        return;
    }

    QStringList draggedData;
    while (!stream.atEnd()) {
        int row, col;
        QMap<int, QVariant> roleData;
        stream >> row >> col >> roleData;
        if (col != 1) {
            continue;
        }
        draggedData.append(roleData[Qt::DisplayRole].toString());
    }

    if (draggedData.isEmpty()) {
        event->ignore();
        return;
    }

    int currentRow = startRow;
    if (currentRow == -1) {
        currentRow = rowCount();
    }
    for (const QString& text : draggedData) {
        if (currentRow >= rowCount()) {
            insertRow(rowCount());
        }
        QString cur = startCol == 1 ? Util::Join(GlobalData::Ins()->GetLocalRoot(), text)
                                    : Util::Join(GlobalData::Ins()->GetRemoteRoot(), text);
        QTableWidgetItem* item = this->item(currentRow, startCol);
        if (!item) {
            item = new QTableWidgetItem(cur);
            setItem(currentRow, startCol, item);
        } else {
            item->setText(cur);
        }
        currentRow++;
    }

    event->acceptProposedAction();
}

void CpTableWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}
