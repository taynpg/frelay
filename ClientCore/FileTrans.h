#ifndef FILETRANS_H
#define FILETRANS_H

#include <QFile>
#include <QMap>
#include <QVector>

#include "ClientCore.h"

struct TransTask {
    bool isUpload{false};
    QString localId;
    QString localPath;
    QString localUUID;
    QString remoteId;
    QString remotePath;
    QString remoteUUID;
};

enum class TaskState {
    STATE_READY = 0,
    STATE_RUNNING,
    STATE_FAILED,
    STATE_FINISH,
};

struct DoTransTask {
    QFile file;
    TaskState state;
    TransTask task;
};

class FileTrans : public QObject
{
    Q_OBJECT
public:
    FileTrans(ClientCore* clientCore);

public:
    void SetTasks(const QVector<TransTask>& tasks);
    void RegisterFrameCall();

private:
    void fbtReqSend(QSharedPointer<FrameBuffer> frame);

private:
    DoTransTask downTask_;
    QVector<TransTask> tasks_;
    ClientCore* clientCore_;
    QMap<QString, DoTransTask> upTasks_;
};

#endif