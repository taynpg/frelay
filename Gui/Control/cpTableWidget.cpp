#include "cpTableWidget.h"

#include <QApplication>
#include <QDrag>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <Util.h>
#include <functional>

#include "GuiUtil/Public.h"

CpTableWidget::CpTableWidget(QWidget* parent) : QTableWidget(parent)
{
}

CpTableWidget::~CpTableWidget()
{
}

void CpTableWidget::setIsResource(bool isResource)
{
    isResource_ = isResource;
}

void CpTableWidget::dropEvent(QDropEvent* event)
{
    if (!isResource_) {
        QMessageBox::information(this, tr("提示"), tr("请先重置搜索结果后操作。"));
        event->ignore();
        return;
    }

    if (!event->mimeData()->hasFormat("application/x-qabstractitemmodeldatalist")) {
        event->ignore();
        return;
    }
    QByteArray encoded = event->mimeData()->data("application/x-qabstractitemmodeldatalist");
    QDataStream stream(&encoded, QIODevice::ReadOnly);

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QPoint pos = event->position().toPoint();
#else
    QPoint pos = event->pos();
#endif

    int startRow = rowAt(pos.y());
    int startCol = columnAt(pos.x());
    if (startCol != 1 && startCol != 2) {
        event->ignore();
        return;
    }

    QStringList parseData;
    while (!stream.atEnd()) {
        int row, col;
        QMap<int, QVariant> roleData;
        stream >> row >> col >> roleData;
        parseData.append(roleData[Qt::DisplayRole].toString());
    }
    if (parseData.isEmpty()) {
        event->ignore();
        return;
    }

    int currentRow = startRow;
    if (currentRow == -1) {
        currentRow = rowCount();
    }

    auto setItemData = [this](int row, int col, const QString& data) {
        QTableWidgetItem* item = this->item(row, col);
        if (!item) {
            item = new QTableWidgetItem(data);
            setItem(row, col, item);
        } else {
            item->setText(data);
        }
    };

    for (int i = 0; i < (parseData.size() / 5); ++i, ++currentRow) {
        if (currentRow >= rowCount()) {
            insertRow(rowCount());
        }
        if (parseData[i * 5 + 3] == "Dir") {
            if (startCol == 1) {
                setItemData(currentRow, startCol, Util::Join(GlobalData::Ins()->GetLocalRoot(), parseData[i * 5 + 1]));
            } else {
                setItemData(currentRow, startCol, Util::Join(GlobalData::Ins()->GetRemoteRoot(), parseData[i * 5 + 1]));
            }
            continue;
        }
        setItemData(currentRow, 0, parseData[i * 5 + 1]);
        if (startCol == 1) {
            setItemData(currentRow, 1, GlobalData::Ins()->GetLocalRoot());
        } else {
            setItemData(currentRow, 2, GlobalData::Ins()->GetRemoteRoot());
        }
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
