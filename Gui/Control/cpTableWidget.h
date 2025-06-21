#ifndef CP_TABLEWIDET_H
#define CP_TABLEWIDET_H

#include <QAction>
#include <QDropEvent>
#include <QMenu>
#include <QTableWidget>

class CpTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    explicit CpTableWidget(QWidget* parent = nullptr);
    ~CpTableWidget() override;

protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event);
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void deleteSelectedRows();

private:
    QMenu* contexMenu_;
    QAction* delAction_;
};

#endif   // CP_TABLEWIDET_H
