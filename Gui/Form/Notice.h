#ifndef NOTICE_H
#define NOTICE_H

#include <QDialog>

// 在头文件中定义宏
#define SHOW_NOTICE(parent, message)                                                                                             \
    Notice* nt = new Notice((parent), (message));                                                                                \
    nt->exec();                                                                                                                  \
    nt->deleteLater();

#define SHOW_NOTICE_TITLE(parent, title, message)                                                                                \
    Notice* nt = new Notice((parent), (title), (message));                                                                       \
    nt->exec();                                                                                                                  \
    nt->deleteLater();

namespace Ui {
class Notice;
}

class Notice : public QDialog
{
    Q_OBJECT

public:
    explicit Notice(QWidget* parent = nullptr);
    explicit Notice(QWidget* parent, const QString& message);
    explicit Notice(QWidget* parent, const QString& title, const QString& message);
    ~Notice();

    // 设置标题和内容
    void setTitle(const QString& title);
    void setMessage(const QString& message);

private slots:
    void btnExit();

private:
    void BaseInit();

private:
    Ui::Notice* ui;
};

#endif   // NOTICE_H
