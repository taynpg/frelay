#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <ClientCore.h>
#include <FileTrans.h>
#include <QDialog>
#include <QFile>
#include <QThread>
#include <unordered_map>

namespace Ui {
class TransForm;
}

class TranFromTh;
class TransForm : public QDialog
{
    Q_OBJECT

public:
    explicit TransForm(QWidget* parent = nullptr);
    ~TransForm();

public:
    void SetClientCore(ClientCore* clientCore);
    void SetTasks(const QVector<TransTask>& tasks);
    void startTask();

signals:
    void sigProgress(double val);
    void sigFailed();
    void sigDone();
    void sigSetUi(const TransTask& task);
    void sigTaskNum(const QString& data);

private:
    void setProgress(double val);
    void handleFailed();
    void handleDone();
    void handleUI(const TransTask& task);
    void showNum(const QString& data);

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    bool exis_{false};
    TranFromTh* workTh_{};
    qint32 curTaskNum_{0};
    QVector<TransTask> tasks_;
    FileTrans* fileTrans_{};
    ClientCore* clientCore_{};
    Ui::TransForm* ui;
};

class TranFromTh : public QThread
{
    Q_OBJECT

public:
    explicit TranFromTh(TransForm* tf, QObject* parent = nullptr) : QThread(parent), tf_(tf)
    {
    }

protected:
    void run() override
    {
        if (tf_) {
            tf_->startTask();
        }
    }

private:
    TransForm* tf_;
};

class CheckCondition : public QThread
{
    Q_OBJECT

public:
    CheckCondition(QObject* parent = nullptr);

public:
    void SetClientCore(ClientCore* clientCore);
    void SetTasks(const QVector<TransTask>& tasks);
    InfoMsg GetInfoMsg() const;

Q_SIGNALS:
    void sigCheckOver();

public Q_SLOTS:
    void interrupCheck();
    void recvFrame(QSharedPointer<FrameBuffer> frame);

protected:
    void run() override;

private:
    QString msg_;
    bool isRun_;
    bool isAlreadyInter_;
    QVector<TransTask> tasks_;
    ClientCore* clientCore_{};
    InfoMsg infoMsg_;
};

#endif // TRANSFORM_H