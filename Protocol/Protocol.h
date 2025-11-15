#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <QMetaType>
#include <QSharedPointer>
#include <QString>
#include <functional>

// 一帧包大小
constexpr quint32 CHUNK_BUF_SIZE = 1 * 1024 * 512;
// 流量背压倍率
constexpr quint32 FLOW_BACK_MULTIPLE = 50;
// 阻塞等级放大倍率
constexpr quint32 BLOCK_LEVEL_MULTIPLE = 5;
// 允许最大的无效数据包大小
constexpr quint32 MAX_INVALID_PACKET_SIZE = CHUNK_BUF_SIZE * 5;

// It is specified here that the first 30 contents (inclusive) are
// used for communication with the server.
// Contents beyond 30 are only forwarded.
enum FrameBufferType : uint16_t {
    FBT_NONE = 0,
    FBT_SER_MSG_HEARTBEAT,
    FBT_SER_MSG_ASKCLIENTS,
    FBT_SER_MSG_YOURID,
    FBT_SER_MSG_RESPONSE,
    FBT_SER_MSG_FORWARD_FAILED,
    FBT_SER_MSG_JUDGE_OTHER_ALIVE,
    FBT_SER_MSG_OFFLINE,
    FBT_SER_FLOW_LIMIT,
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
    FBT_CLI_TRANS_INTERRUPT,
    FBT_CLI_FILE_INFO,
    FBT_MSGINFO_ASK,
    FBT_MSGINFO_ANSWER
};

struct FrameBuffer {
    QByteArray data;
    QString fid;
    QString tid;
    FrameBufferType type = FBT_NONE;
    bool sendRet;
};

// 拥堵等级，越高越堵
enum BlockLevel {
    BL_LEVEL_NORMAL = 0,
    BL_LEVEL_1 = 1,
    BL_LEVEL_2 = 3,
    BL_LEVEL_3 = 5,
    BL_LEVEL_4 = 10,
    BL_LEVEL_5 = 20,
    BL_LEVEL_6 = 50,
    BL_LEVEL_7 = 100,
    BL_LEVEL_8 = 1000
};

class Protocol
{
public:
    Protocol();

public:
    static QSharedPointer<FrameBuffer> ParseBuffer(QByteArray& buffer);
    static QByteArray PackBuffer(const QSharedPointer<FrameBuffer>& frame);
};

enum FileCheckState {
    FCS_NORMAL = 0,
    FCS_DIR_NOT_EXIST,
    FCS_FILE_NOT_EXIST,
    FCS_FILE_EXIST
};

// 字符串标识
#define STRMSG_AC_CHECK_FILE_EXIST "requestCheckFileExist"
#define STRMSG_AC_ANSWER_FILE_EXIST "answerCheckFileExist"
#define STRMSG_AC_DEL_FILE "requestDelFile"
#define STRMSG_AC_ANSWER_DEL_FILE "answerDelFile"
#define STRMSG_AC_DEL_DIR "requestDelDir"
#define STRMSG_AC_ANSWER_DEL_DIR "answerDelDir"
#define STRMSG_AC_RENAME_FILEDIR "requestRenameFileDir"
#define STRMSG_AC_ANSWER_FILEDIR "answerRenameFileDir"
#define STRMSG_AC_NEW_DIR "requestNewDir"
#define STRMSG_AC_ANSWER_NEW_DIR "answerNewDir"
#define STRMSG_AC_ASK_FILEINFO "requestFileInfo"
#define STRMSG_AC_ANSWER_FILEINFO "answerFileInfo"
#define STRMSG_AC_UP "upAction"
#define STRMSG_AC_DOWN "downAction"
#define STRMSG_AC_ACCEPT "acceptAction"
#define STRMSG_AC_REJECT "rejectAction"
#define STRMSG_AC_CANCEL "cancelAction"

#define STRMSG_ST_FILEEXIT "fileExist"
#define STRMSG_ST_FILENOEXIT "fileNotExist"
#define STRMSG_ST_DIREXIT "dirExist"
#define STRMSG_ST_DIRNOEXIT "dirNotExist"

#endif   // PROTOCOL_H