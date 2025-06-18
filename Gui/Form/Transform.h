#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <ClientCore.h>
#include <QDialog>
#include <QFile>
#include <FileTrans.h>

namespace Ui {
class TransForm;
}

class TransForm : public QDialog
{
    Q_OBJECT

public:
    explicit TransForm(QWidget* parent = nullptr);
    ~TransForm();

public:
    void SetClientCore(ClientCore* clientCore);
    void SetTasks(const QVector<TransTask>& tasks);

private:
    void startTask();

protected:
    void showEvent(QShowEvent* event) override;

private:
    QVector<TransTask> tasks_;
    FileTrans* fileTrans_{};
    ClientCore* clientCore_{};
    Ui::TransForm* ui;
};

#endif   // TRANSFORM_H
