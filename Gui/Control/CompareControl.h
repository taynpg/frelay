﻿#ifndef COMPARECONTROL_H
#define COMPARECONTROL_H

#include <FileTrans.h>
#include <QMenu>
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
    void sigTryVisit(bool local, const QString& path);

private:
    void InitControl();
    void InitTabWidget();
    void InitMenu();

private:
    void Save();
    void Load();
    void LoadTitles();

    void TransToLeft();
    void TransToRight();

private slots:
    void deleteSelectedRows();

private:
    QMenu* menu_;
    Ui::Compare* ui;
};

#endif   // COMPARECONTROL_H
