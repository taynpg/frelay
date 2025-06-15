#include "Protocol.h"

#include <QIODevice>
#include <cstdint>

Protocol::Protocol()
{
}

QSharedPointer<FrameBuffer> Protocol::ParseBuffer(QByteArray& buffer)
{
    static QByteArray protocolHeader = QByteArray::fromRawData("\xFF\xFE", 2);
    static QByteArray protocolTail = QByteArray::fromRawData("\xFF\xFF", 2);

    int headerPos = buffer.indexOf(protocolHeader);
    if (headerPos == -1) {
        return nullptr;
    }

    const int minFrameSize = protocolHeader.size() + sizeof(uint16_t) + 32 + 32 + sizeof(uint32_t) + protocolTail.size();
    if (buffer.size() - headerPos < minFrameSize) {
        return nullptr;
    }

    QDataStream stream(buffer.mid(headerPos));
    stream.setByteOrder(QDataStream::LittleEndian);

    QByteArray header;
    header.resize(protocolHeader.size());
    stream.readRawData(header.data(), protocolHeader.size());
    if (header != protocolHeader) {
        return nullptr;
    }

    FrameBufferType type;
    stream >> reinterpret_cast<quint16&>(type);

    QByteArray fidBytes(32, '\0');
    QByteArray tidBytes(32, '\0');
    stream.readRawData(fidBytes.data(), 32);
    stream.readRawData(tidBytes.data(), 32);

    auto b2id = [](const QByteArray& binary, int fixLen) {
        const int nullPos = binary.indexOf('\0');
        const int len = (nullPos >= 0) ? qMin(nullPos, fixLen) : fixLen;
        return QString::fromUtf8(binary.constData(), len);
    };

    quint32 dataLen;
    stream >> dataLen;

    int totalFrameSize = minFrameSize - protocolTail.size() + dataLen;
    if (buffer.size() - headerPos < totalFrameSize) {
        return nullptr;
    }
    QByteArray data;
    if (dataLen > 0) {
        data.resize(dataLen);
        stream.readRawData(data.data(), dataLen);
    }
    QByteArray tail;
    tail.resize(protocolTail.size());
    stream.readRawData(tail.data(), protocolTail.size());
    if (tail != protocolTail) {
        return nullptr;
    }
    auto frame = QSharedPointer<FrameBuffer>::create();
    frame->type = type;
    frame->fid = b2id(fidBytes, 32);
    frame->tid = b2id(tidBytes, 32);
    frame->data = data;
    buffer.remove(0, headerPos + totalFrameSize);
    return frame;
}

QByteArray Protocol::PackBuffer(const QSharedPointer<FrameBuffer>& frame)
{
    static QByteArray protocolHeader = QByteArray::fromRawData("\xFF\xFE", 2);
    static QByteArray protocolTail = QByteArray::fromRawData("\xFF\xFF", 2);

    if (frame.isNull()) {
        return QByteArray();
    }

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.writeRawData(protocolHeader.constData(), protocolHeader.size());
    stream << static_cast<quint16>(frame->type);

    QByteArray fidBytes = frame->fid.toUtf8().leftJustified(32, '\0', true);
    stream.writeRawData(fidBytes.constData(), 32);
    QByteArray tidBytes = frame->tid.toUtf8().leftJustified(32, '\0', true);
    stream.writeRawData(tidBytes.constData(), 32);

    stream << static_cast<quint32>(frame->data.size());
    if (!frame->data.isEmpty()) {
        stream.writeRawData(frame->data.constData(), frame->data.size());
    }
    stream.writeRawData(protocolTail.constData(), protocolTail.size());
    return packet;
}
