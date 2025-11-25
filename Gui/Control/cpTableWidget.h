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

public:
    void setIsResource(bool isResource);

protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;

private:
    bool isResource_{};
};

#endif   // CP_TABLEWIDET_H
