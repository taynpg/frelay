#include "cusTableWidget.h"
#include <QDrag>
#include <QMimeData>
#include <QPainter>

CustomTableWidget::CustomTableWidget(QWidget* parent)
{
}

CustomTableWidget::~CustomTableWidget()
{
}

void CustomTableWidget::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasFormat("application/x-custom-data")) {
        event->ignore();
        return;
    }

    QModelIndex index = indexAt(event->pos());
    if (!index.isValid()) {
        event->ignore();
        return;
    }

    QString data = QString::fromUtf8(event->mimeData()->data("application/x-custom-data"));
    QStringList items = data.split(",");

    //emit customDropRequested(items.first(), index.row(), index.column(), event->source());

    event->acceptProposedAction();
}

void CustomTableWidget::dragMoveEvent(QDragMoveEvent* event)
{
    const QTableWidget* source = qobject_cast<const QTableWidget*>(event->source());
    if (source == this) {
        event->ignore();
        return;
    }
    if (event->mimeData()->hasFormat("application/x-custom-data")) {
        event->setDropAction(Qt::MoveAction);
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void CustomTableWidget::startDrag(Qt::DropActions supportedActions)
{
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;

    QStringList selectedTexts;
    foreach (QTableWidgetItem* item, selectedItems()) {
        selectedTexts << item->text();
    }
    mimeData->setData("application/x-custom-data", selectedTexts.join(",").toUtf8());
    drag->setMimeData(mimeData);
    QPixmap pixmap(100, 50);
    pixmap.fill(Qt::lightGray);
    QPainter painter(&pixmap);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, QString("%1 ITEM").arg(selectedTexts.count()));
    drag->setPixmap(pixmap);
    drag->exec(supportedActions);
}
