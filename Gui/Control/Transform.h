#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <ClientCore.h>
#include <QDialog>

namespace Ui {
class TransForm;
}

struct TransTask {
    bool isUpload{false};
    QString localId;
    QString localPath;
    QString remoteId;
    QString remotePath;
};

enum class TaskState {
    STATE_READY = 0,
    STATE_RUNNING,
    STATE_FAILED,
    STATE_FINISH,
};

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
    void StartExecTask();
    void StopExecTask();

private:
    TaskState curState_{TaskState::STATE_READY};
    QVector<TransTask> tasks_;
    ClientCore* clientCore_;
    Ui::TransForm* ui;
};

#endif   // TRANSFORM_H
