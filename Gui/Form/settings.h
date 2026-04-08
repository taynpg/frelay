#ifndef SETTINGS_H
#define SETTINGS_H

#include <GuiUtil/Config.h>
#include <QDialog>

namespace Ui {
class Settings;
}

class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(QWidget* parent = nullptr);
    ~Settings();

public:
    void SetConfigPtr(std::shared_ptr<FrelayConfig> config);

private:
    void InitContrl();

private slots:
    void changeTheme(const QString& theme);

private:
    std::shared_ptr<FrelayConfig> config_;
    Ui::Settings* ui;
};

#endif   // SETTINGS_H
