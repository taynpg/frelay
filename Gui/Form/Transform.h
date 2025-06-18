#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <ClientCore.h>
#include <QDialog>
#include <QFile>

namespace Ui {
class TransForm;
}

class TransForm : public QDialog
{
    Q_OBJECT

public:
    explicit TransForm(QWidget* parent = nullptr);
    ~TransForm();

public:
    void SetClientCore(ClientCore* clientCore);

private:
    ClientCore* clientCore_;
    Ui::TransForm* ui;
};

#endif   // TRANSFORM_H
