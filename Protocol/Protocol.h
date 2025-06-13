#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QSharedPointer>
#include <QString>

// It is specified here that the first 30 contents (inclusive) are
// used for communication with the server.
// Contents beyond 30 are only forwarded.
enum FrameBufferType : uint16_t {
    FBT_SER_MSG_ASKCLIENTS = 0,
    FBT_SER_MSG_YOURID,
    FBT_SER_MSG_RESPONSE,
    FBT_SER_MSG_FORWARD_FAILED,
    FBT_CLI_BIN_FILEDATA = 31,
    FBT_CLI_MSG_COMMUNICATE,
    FBT_CLI_ASK_DIRFILE,
    FBT_CLI_ANS_DIRFILE,
    FBT_CLI_ASK_HOME,
    FBT_CLI_ANS_HOME
};

struct FrameBuffer {
    QByteArray data;
    QString fid;
    QString tid;
    FrameBufferType type;
};

class Protocol
{
public:
    Protocol();

public:
    static QSharedPointer<FrameBuffer> ParseBuffer(QByteArray& buffer);
    static QByteArray PackBuffer(const QSharedPointer<FrameBuffer>& frame);
};

#endif   // PROTOCOL_H
