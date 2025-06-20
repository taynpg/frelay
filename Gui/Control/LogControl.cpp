#include "LogControl.h"

#include <QDateTime>
#include <QListWidgetItem>
#include <QStandardItem>
#include <iomanip>
#include <sstream>

#include "ui_LogControl.h"

LogPrint::LogPrint(QWidget* parent) : QWidget(parent), ui(new Ui::LogPrint)
{
    ui->setupUi(this);
    InitControl();
}

void LogPrint::InitControl()
{
    ui->pedText->setReadOnly(true);
}

std::string LogPrint::now_str()
{
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream timestamp;
    timestamp << std::put_time(std::localtime(&time_t_now), "%H:%M:%S") << "." << std::setfill('0') << std::setw(3)
              << milliseconds.count() << " ";

    return timestamp.str();
}

LogPrint::~LogPrint()
{
    delete ui;
}

void LogPrint::Info(const QString& message)
{
    Print(message, Qt::black);
}
void LogPrint::Warn(const QString& message)
{
    Print(message, Qt::gray);
}
void LogPrint::Error(const QString& message)
{
    Print(message, Qt::red);
}
void LogPrint::Debug(const QString& message)
{
    Print(message, Qt::blue);
}
void LogPrint::Print(const QString& message, const QBrush& color)
{
    QString timeStr = QString("%1%2").arg(QString::fromStdString(now_str())).arg(message);

    QString coloredLog = QString("<span style='color:%1;'>%2</span>")
                             .arg(color.color().name())
                             .arg(timeStr.toHtmlEscaped());
    ui->pedText->appendHtml(coloredLog);
}
