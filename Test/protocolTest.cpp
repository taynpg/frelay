#include <Protocol.h>
#include <QDebug>

#include "infoTest.h"
#include "msgTest.h"

int test1()
{
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->type = FBT_CLI_BIN_FILEDATA;
    frame->fid = "client123";
    frame->tid = "file456";
    frame->data = QByteArray("This is test binary data", 24);

    QByteArray packet = Protocol::PackBuffer(frame);
    qDebug() << "Test1 - Packed data size:" << packet.size();
    qWarning() << "Packed data hex:" << packet.toHex();

    auto ret = Protocol::ParseBuffer(packet);
    return 0;
}

int main()
{
    //setConsoleMsgHandler();
    infoTest3();
    return 0;
}
