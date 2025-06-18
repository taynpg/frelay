#include "cusTableWidget.h"

#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QPainter>

CustomTableWidget::CustomTableWidget(QWidget* parent) : QTableWidget(parent)
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

    QStringList selectedTexts;
    foreach (QTableWidgetItem* item, selectedItems()) {
        if (item->column() == 1) {
            selectedTexts << item->text();
        }
    }
    mimeData->setData("application/x-custom-data", selectedTexts.join("|").toUtf8());
    drag->setMimeData(mimeData);
    QPixmap pixmap(100, 50);
    pixmap.fill(Qt::lightGray);
    QPainter painter(&pixmap);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, QString("%1 ITEM").arg(selectedTexts.count()));
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
