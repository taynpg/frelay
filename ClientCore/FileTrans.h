#ifndef FILETRANS_H
#define FILETRANS_H

#include <QFile>
#include <QMap>
#include <QMutex>
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
    STATE_NONE = 0,
    STATE_RUNNING,
    STATE_FAILED,
    STATE_FINISH,
};

struct DoTransTask {
    QFile file;
    TaskState state = TaskState::STATE_NONE;
    TransTask task;
};

class SendThread : public QThread
{
    Q_OBJECT
public:
    SendThread(ClientCore* clientCore);

public:
    void run() override;
    void setTask(const QSharedPointer<DoTransTask>& task);

private:
    ClientCore* cliCore_;
    QSharedPointer<DoTransTask> task_;
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
    void fbtReqRecv(QSharedPointer<FrameBuffer> frame);
    void fbtTransDone(QSharedPointer<FrameBuffer> frame);
    void fbtAnsRecvSuccess(QSharedPointer<FrameBuffer> frame);
    void fbtAnsRecvFailed(QSharedPointer<FrameBuffer> frame);
    void fbtFileTrans(QSharedPointer<FrameBuffer> frame);
    void fbtAnsSendFailed(QSharedPointer<FrameBuffer> frame);
    void fbtAnsSendSuccess(QSharedPointer<FrameBuffer> frame);
    void fbtFileTransFailed(QSharedPointer<FrameBuffer> frame);

private:
    void SendFile(const QSharedPointer<DoTransTask>& task);

private:
    DoTransTask downTask_;

    QMutex lMut_;
    QMutex rMut_;
    QVector<TransTask> localTasks_;
    QVector<TransTask> remoteTasks_;

    ClientCore* clientCore_;
    QMutex sthMut_;
    QVector<QThread*> sendThreads_;
    QMap<QString, QThread*> upTasks_;
};

#endif