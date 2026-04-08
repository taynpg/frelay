#ifndef FRELAY_THEME_H
#define FRELAY_THEME_H

#include <QStyle>

#ifdef _WIN32
#ifdef THEME_EXPORT
#define THEME_API __declspec(dllexport)
#else
#define THEME_API __declspec(dllimport)
#endif
#else
#define THEME_API __attribute__((visibility("default")))
#endif

class THEME_API Theme
{
public:
    Theme() = delete;
    ~Theme() = delete;

public:
    static QStyle* getStyle(QObject* parent = nullptr);
};

#endif   // FRELAY_THEME_H