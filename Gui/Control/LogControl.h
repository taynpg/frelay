#ifndef LOGCONTROL_H
#define LOGCONTROL_H

#include <QBrush>
#include <QStandardItemModel>
#include <QWidget>

namespace Ui {
class LogPrint;
}

class LogPrint : public QWidget
{
    Q_OBJECT

public:
    explicit LogPrint(QWidget* parent = nullptr);
    ~LogPrint();

public:
    void Info(const QString& message);
    void Warn(const QString& message);
    void Error(const QString& message);
    void Debug(const QString& message);

public:
    std::string now_str();

private:
    void InitControl();
    void Print(const QString& message, const QBrush& color);

private:
    Ui::LogPrint* ui;
    QStandardItemModel* model_;
};

#endif   // LOGCONTROL_H
