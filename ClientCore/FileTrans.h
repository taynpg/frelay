#ifndef FILETRANS_H
#define FILETRANS_H

#include <QFile>
#include <QFuture>
#include <QMap>
#include <QMutex>
#include <QVector>

#include "ClientCore.h"

constexpr int MAX_SEND_TASK = 10;

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
    quint64 totalSize{0};
    quint64 tranSize{0};
    quint32 permission{};
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
    bool isSuccess_{false};
    ClientCore* cliCore_;
    quint32 curSendCount_{0};
    QSharedPointer<DoTransTask> task_;
};

class FileTrans : public QObject
{
    Q_OBJECT

public:
    FileTrans(ClientCore* clientCore);

public:
    void ReqSendFile(const TransTask& task);
    void ReqDownFile(const TransTask& task);
    qint32 GetSendProgress();
    qint32 GetDownProgress();

private:
    void fbtReqSend(QSharedPointer<FrameBuffer> frame);
    void fbtReqDown(QSharedPointer<FrameBuffer> frame);
    void fbtTransDone(QSharedPointer<FrameBuffer> frame);
    void fbtCanDown(QSharedPointer<FrameBuffer> frame);
    void fbtCanotDown(QSharedPointer<FrameBuffer> frame);
    void fbtFileBuffer(QSharedPointer<FrameBuffer> frame);
    void fbtFileInfo(QSharedPointer<FrameBuffer> frame);
    void fbtCanotSend(QSharedPointer<FrameBuffer> frame);
    void fbtCanSend(QSharedPointer<FrameBuffer> frame);
    void fbtTransFailed(QSharedPointer<FrameBuffer> frame);
    void fbtInterrupt(QSharedPointer<FrameBuffer> frame);

private:
    void RegisterSignal();
    void SendFile(const QSharedPointer<DoTransTask>& task);

private:
    QSharedPointer<DoTransTask> downTask_;
    QSharedPointer<DoTransTask> sendTask_;

    QMutex lMut_;
    QMutex rMut_;

    ClientCore* clientCore_;
    QMutex sthMut_;
    QMap<QString, QThread*> upTasks_;
};

#endif