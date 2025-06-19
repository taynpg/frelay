#ifndef CUSTOM_TABLEWIDET_H
#define CUSTOM_TABLEWIDET_H

#include <FileTrans.h>
#include <QDropEvent>
#include <QTableWidget>

class CustomTableWidget : public QTableWidget
{
    Q_OBJECT
public:
    explicit CustomTableWidget(QWidget* parent = nullptr);
    ~CustomTableWidget() override;

signals:
    void sigTasks(const QVector<TransTask>& tasks);

public:
    void setIsRemote(bool isRemote);
    void setBasePathCall(const std::function<QString()>& call);
    void setOwnIDCall(const std::function<QString()>& call);
    void setRemoteIDCall(const std::function<QString()>& call);
    void setOtherSideCall(const std::function<QString()>& call);

protected:
    void dropEvent(QDropEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event);

protected:
    bool isRemote_{false};
    QPoint startPos_;
    std::function<QString()> basePathCall_;
    std::function<QString()> otherSideCall_;
    std::function<QString()> oidCall_;
    std::function<QString()> ridCall_;
};

#endif
