#ifndef COMMON_H
#define COMMON_H

#include <QWidget>

class Common : public QWidget
{
    Q_OBJECT
public:
    Common();

public:
    static int GetAcceptThree(QWidget* parent, const QString& notice, const QString& title);
};

#endif // COMMON_H