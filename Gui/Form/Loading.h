#ifndef LOADINGDIALOG_H
#define LOADINGDIALOG_H

#include <QDialog>
#include <QGraphicsDropShadowEffect>
#include <QLabel>
#include <QMovie>
#include <QPainter>
#include <QPushButton>
#define USER_CANCEL -1

// LoadingDialog 改造来源：https://blog.csdn.net/weixin_42105886/article/details/114665272
class LoadingDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoadingDialog(QWidget* parent = nullptr);
    ~LoadingDialog();
    // 设置提示文本
    void setTipsText(QString strTipsText);
    // 设置是否显示取消等待按钮
    void setCanCancel(bool bCanCancel);
    // 移动到指定窗口中间显示
    void moveToCenter(QWidget* pParent);

public:
    int exec() override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void initUi();

Q_SIGNALS:
    void cancelWaiting();

public slots:
    void cancelBtnClicked();

private:
    QFrame* m_pCenterFrame;
    QLabel* m_pMovieLabel;
    QMovie* m_pLoadingMovie;
    QLabel* m_pTipsLabel;
    bool isShow_{};
    QPushButton* m_pCancelBtn;
};
#endif   // LOADINGDIALOG_H