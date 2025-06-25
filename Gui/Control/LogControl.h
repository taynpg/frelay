#ifndef LOGCONTROL_H
#define LOGCONTROL_H

#include <QBrush>
#include <QGuiApplication>
#include <QMap>
#include <QMutex>
#include <QStandardItemModel>
#include <QStyleHints>
#include <QWidget>

namespace Ui {
class LogPrint;
}

enum PrintType { PT_INFO, PT_WARN, PT_ERROR, PT_DEBUG };

struct LogEntry {
    QString message;
    PrintType level;
};

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

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    void ColorChange(Qt::ColorScheme scheme);
#endif

private:
    void InitControl();
    void InitColor();
    void RePrintLog();
    void Print(const QString& message, PrintType type, bool recordHis = true);

private:
    QMutex mutex_;
    bool isLightMode_{true};
    Ui::LogPrint* ui;
    QMap<PrintType, QBrush> lightMap_;
    QMap<PrintType, QBrush> darkMap_;
    QVector<LogEntry> logEntries_;

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QStyleHints* styleHints_{};
#endif

    QStandardItemModel* model_;
};

#endif   // LOGCONTROL_H
