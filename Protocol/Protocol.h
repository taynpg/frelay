#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QMetaType>
#include <QSharedPointer>
#include <QString>
#include <functional>

constexpr quint32 CHUNK_BUF_SIZE = 1 * 1024 * 1024;

// It is specified here that the first 30 contents (inclusive) are
// used for communication with the server.
// Contents beyond 30 are only forwarded.
enum FrameBufferType : uint16_t {
    FBT_NONE = 0,
    FBT_SER_MSG_ASKCLIENTS,
    FBT_SER_MSG_YOURID,
    FBT_SER_MSG_RESPONSE,
    FBT_SER_MSG_FORWARD_FAILED,
    FBT_CLI_BIN_FILEDATA = 31,
    FBT_CLI_MSG_COMMUNICATE,
    FBT_CLI_ASK_DIRFILE,
    FBT_CLI_ANS_DIRFILE,
    FBT_CLI_ASK_HOME,
    FBT_CLI_ANS_HOME,
    FBT_CLI_REQ_SEND,
    FBT_CLI_CAN_SEND,
    FBT_CLI_CANOT_SEND,
    FBT_CLI_REQ_DOWN,
    FBT_CLI_CAN_DOWN,
    FBT_CLI_CANOT_DOWN,
    FBT_CLI_FILE_BUFFER,
    FBT_CLI_TRANS_DONE,
    FBT_CLI_TRANS_FAILED,
    FBT_CLI_FILE_INFO
};

struct FrameBuffer {
    QByteArray data;
    QString fid;
    QString tid;
    FrameBufferType type = FBT_NONE;
    bool sendRet;
    std::function<void(QSharedPointer<FrameBuffer>)> call{};
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
