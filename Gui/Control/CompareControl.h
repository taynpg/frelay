#ifndef COMPARECONTROL_H
#define COMPARECONTROL_H

#include <FileTrans.h>
#include <QMenu>
#include <QVector>
#include <QWidget>

namespace Ui {
class Compare;
}

struct CompareItem {
    QString baseName;
    QString localDir;
    QString remoteDir;
};

class Compare : public QWidget
{
    Q_OBJECT

public:
    explicit Compare(QWidget* parent = nullptr);
    ~Compare();

signals:
    void sigTasks(const QVector<TransTask>& tasks);
    void sigTryVisit(bool local, const QString& path);

private:
    void InitControl();
    void InitTabWidget();
    void InitMenu();

private:
    void Save();
    void Load();
    void LoadTitles();
    void Search();
    void Reset();
    void SetResult(const QVector<CompareItem>& items);

    void TransToLeft();
    void TransToRight();

private slots:
    void deleteSelectedRows();

private:
    QMenu* menu_;
    Ui::Compare* ui;

    // 现要求，保存、删除、拖入必须重置。
    bool isResource_{};
    QVector<CompareItem> items_;
};

#endif   // COMPARECONTROL_H
