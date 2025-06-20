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
    InitColor();
}

void LogPrint::InitControl()
{
    isLightMode_ = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light;
    styleHints_ = QGuiApplication::styleHints();
    connect(styleHints_, &QStyleHints::colorSchemeChanged, this, &LogPrint::ColorChange);
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

void LogPrint::ColorChange(Qt::ColorScheme scheme)
{
    if (scheme == Qt::ColorScheme::Dark) {
        isLightMode_ = false;
    } else {
        isLightMode_ = true;
    }
    RePrintLog();
}

LogPrint::~LogPrint()
{
    delete ui;
}

static const QColor DARK_INFO_COLOR = QColor(255, 255, 0);
static const QColor DARK_WARN_COLOR = QColor(255, 102, 0);
static const QColor DARK_ERROR_COLOR = QColor(255, 0, 255);
static const QColor DARK_DEBUG_COLOR = QColor(0, 255, 255);

void LogPrint::InitColor()
{
    lightMap_[PT_INFO] = Qt::black;
    lightMap_[PT_WARN] = Qt::blue;
    lightMap_[PT_ERROR] = Qt::red;
    lightMap_[PT_DEBUG] = Qt::gray;

    darkMap_[PT_INFO] = DARK_INFO_COLOR;
    darkMap_[PT_WARN] = DARK_WARN_COLOR;
    darkMap_[PT_ERROR] = DARK_ERROR_COLOR;
    darkMap_[PT_DEBUG] = DARK_DEBUG_COLOR;
}

void LogPrint::RePrintLog()
{
    {
        QMutexLocker locker(&mutex_);
        ui->pedText->clear();
    }

    for (auto& entry : logEntries_) {
        Print(entry.message, entry.level, false);
    }
}

void LogPrint::Info(const QString& message)
{
    Print(message, PT_INFO);
}
void LogPrint::Warn(const QString& message)
{
    Print(message, PT_WARN);
}
void LogPrint::Error(const QString& message)
{
    Print(message, PT_ERROR);
}
void LogPrint::Debug(const QString& message)
{
    Print(message, PT_DEBUG);
}

void LogPrint::Print(const QString& message, PrintType type, bool recordHis)
{
    QMutexLocker locker(&mutex_);
    QString timeStr = QString("%1%2").arg(QString::fromStdString(now_str())).arg(message);
    QString r;
    if (isLightMode_) {
        r = QString("<span style='color:%1;'>%2</span>").arg(lightMap_[type].color().name()).arg(timeStr.toHtmlEscaped());
    } else {
        r = QString("<span style='color:%1;'>%2</span>").arg(darkMap_[type].color().name()).arg(timeStr.toHtmlEscaped());
    }
    if (recordHis) {
        logEntries_.append({message, type});
    }
    ui->pedText->appendHtml(r);
}
