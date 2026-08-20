#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "sample.h"

#include <QByteArray>
#include <QtGlobal>

#include <optional>

namespace Protocol {

constexpr quint8 kHeaderByte0 = 0xA5;
constexpr quint8 kHeaderByte1 = 0x5A;
constexpr quint8 kVersion     = 0x01;

constexpr int kHeaderSize     = 6;                       // 帧头2 + 版本1 + 类型1 + 长度2
constexpr int kCrcSize        = 2;
constexpr int kMinFrameSize   = kHeaderSize + kCrcSize;  // 载荷为空时的帧长
constexpr int kMaxPayloadSize = 4096;

enum class MessageType : quint8
{
    Unknown   = 0x00,
    Sample    = 0x01,
    Heartbeat = 0x02,
};

struct Frame
{
    MessageType type = MessageType::Unknown;
    QByteArray  payload;
};

quint16 crc16(const QByteArray& data);

QByteArray           buildFrame(MessageType type, const QByteArray& payload);
std::optional<Frame> parseFrame(const QByteArray& frame);

QByteArray            encodeSample(const Sample& sample);
std::optional<Sample> decodeSample(const QByteArray& payload);

} // namespace Protocol

#endif // PROTOCOL_H