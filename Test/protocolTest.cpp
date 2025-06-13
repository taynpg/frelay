#include <Protocol.h>
#include <QDebug>

int main()
{
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->type = FBT_CLI_BIN_FILEDATA;
    frame->fid = "client123";
    frame->tid = "file456";
    frame->data = QByteArray("This is test binary data", 24);

    QByteArray packet = Protocol::PackBuffer(frame);
    qDebug() << "Test1 - Packed data size:" << packet.size();
    qDebug() << "Packed data hex:" << packet.toHex();

    auto ret = Protocol::ParseBuffer(packet);

    return 0;
}