#include "settings.h"

#include "Form/Notice.h"
#include "ui_settings.h"

Settings::Settings(QWidget* parent) : QDialog(parent), ui(new Ui::Settings)
{
    ui->setupUi(this);
    InitContrl();
}

Settings::~Settings()
{
    delete ui;
}

void Settings::SetConfigPtr(std::shared_ptr<FrelayConfig> config)
{
    config_ = config;
}

void Settings::InitContrl()
{
    ui->cbTheme->addItem("flat");
    ui->cbTheme->addItem("flatgray");
    ui->cbTheme->addItem("fusion");
    ui->cbTheme->addItem("windows");
    ui->cbTheme->addItem("default");

#ifdef USE_EXTERN_THEME
    ui->cbTheme->addItem("qlementine");
#endif

    QString curTheme;
    config_->GetCurrentTheme(curTheme);

    if (curTheme == "flat" || curTheme == "fusion" || curTheme == "windows" || curTheme == "qlementine" ||
        curTheme == "default" || curTheme == "flatgray") {
        ui->cbTheme->setCurrentText(curTheme);
    } else {
        ui->cbTheme->setCurrentText("windows");
    }

    connect(ui->btnSave, &QPushButton::clicked, this, [this]() { changeTheme(ui->cbTheme->currentText()); });
    connect(ui->btnCancel, &QPushButton::clicked, this, [this]() { close(); });
}

void Settings::changeTheme(const QString& theme)
{
    QString curTheme;
    config_->GetCurrentTheme(curTheme);
    if (curTheme != theme) {
        config_->SetCurrentTheme(theme);
        SHOW_NOTICE(this, "请重启软件以应用主题修改");
    }
    close();
}