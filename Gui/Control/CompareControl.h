#ifndef COMPARECONTROL_H
#define COMPARECONTROL_H

#include <QWidget>

namespace Ui {
class Compare;
}

class Compare : public QWidget
{
    Q_OBJECT

public:
    explicit Compare(QWidget *parent = nullptr);
    ~Compare();

private:
    Ui::Compare *ui;
};

#endif // COMPARECONTROL_H
