#ifndef FRELAY_CONSOLE_H
#define FRELAY_CONSOLE_H

#include <ClientCore.h>

class RelayConsole : public QObject
{
	Q_OBJECT

public:
	RelayConsole(QObject* parent = nullptr);
	~RelayConsole() override;
};

#endif // FRELAY_CONSOLE_H
