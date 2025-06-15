#ifndef PUBLIC_H
#define PUBLIC_H

#include <QString>
#include <QWidget>

class FTCommon
{
public:
    static void msg(QWidget* parent, const QString& content);
    static bool affirm(QWidget* parent, const QString& titile, const QString& content);
    static QString select_file(QWidget* parent, const QString& info, const QString& filter);
    static QString GetAppPath();
};

#endif   // PUBLIC_H 