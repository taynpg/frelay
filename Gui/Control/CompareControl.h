#ifndef COMPARECONTROL_H
#define COMPARECONTROL_H

#include <FileTrans.h>
#include <QVector>
#include <QWidget>

namespace Ui {
class Compare;
}

class Compare : public QWidget
{
    Q_OBJECT

public:
    explicit Compare(QWidget* parent = nullptr);
    ~Compare();

signals:
    void sigTasks(const QVector<TransTask>& tasks);

private:
    void InitControl();
    void InitTabWidget();

private:
    void Save();
    void Load();
    void LoadTitles();

    void TransToLeft();
    void TransToRight();

private:
    Ui::Compare* ui;
};

#endif   // COMPARECONTROL_H
