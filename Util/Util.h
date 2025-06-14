#ifndef UTIL_H
#define UTIL_H

#include <QObject>

class Util : public QObject
{
    Q_OBJECT
public:
    Util();

public:
    static void InitLogger(const QString& logPath, const QString& mark);
    static void ConsoleMsgHander(QtMsgType type, const QMessageLogContext& context, const QString& msg);
};

#endif   // UTIL_H