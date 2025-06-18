#ifndef CUSTOM_TABLEWIDET_H
#define CUSTOM_TABLEWIDET_H

#include <QTableWidget>
#include <QDropEvent>

class CustomTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    explicit CustomTableWidget(QWidget* parent = nullptr);
    ~CustomTableWidget() override;

protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event);
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

protected:
    QPoint startPos_;
};

#endif
