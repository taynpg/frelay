#include "msgTest.h"

void setConsoleMsgHandler()
{
    Util::InitLogger("FrelayTest.log", "FrelayTest");
    qInstallMessageHandler(Util::ConsoleMsgHander);
}