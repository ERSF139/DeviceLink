#include "protocol.h"

#include <QDataStream>
#include <QIODevice>

namespace Protocol {

namespace {

constexpr int kSamplePayloadSize = 4 + 8 + 8 + 8 + 8;   // id + 时间戳 + 三个 double = 36

} // namespace

quint16 readUint16BE(const QByteArray& data, int offset)
{
    const auto high = static_cast<quint8>(data.at(offset));
    const auto low  = static_cast<quint8>(data.at(offset + 1));
    return static_cast<quint16>((high << 8) | low);
}

std::optional<int> expectedPayloadSize(quint8 type)
{
    switch (static_cast<MessageType>(type)) {
    case MessageType::Sample:
        return kSamplePayloadSize;
    case MessageType::Heartbeat:
        return 0;
    case MessageType::Unknown:
        break;
    }
    return std::nullopt;
}

quint16 crc16(const QByteArray& data)
{
    quint16 crc = 0xFFFF;

    for (const char byte : data) {
        crc ^= static_cast<quint8>(byte);

        for (int bit = 0; bit < 8; ++bit) {
            if (crc & 0x0001)
                crc = static_cast<quint16>((crc >> 1) ^ 0xA001);
            else
                crc = static_cast<quint16>(crc >> 1);
        }
    }

    return crc;
}

QByteArray buildFrame(MessageType type, const QByteArray& payload)
{
    QByteArray frame;
    frame.reserve(kMinFrameSize + payload.size());

    {
        QDataStream stream(&frame, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);

        stream << kHeaderByte0
               << kHeaderByte1
               << kVersion
               << static_cast<quint8>(type)
               << static_cast<quint16>(payload.size());
    }

    frame.append(payload);

    const quint16 crc = crc16(frame);
    frame.append(static_cast<char>((crc >> 8) & 0xFF));
    frame.append(static_cast<char>(crc & 0xFF));

    return frame;
}

std::optional<Frame> parseFrame(const QByteArray& frame)
{
    if (frame.size() < kMinFrameSize)
        return std::nullopt;

    if (static_cast<quint8>(frame.at(0)) != kHeaderByte0
        || static_cast<quint8>(frame.at(1)) != kHeaderByte1)
        return std::nullopt;

    if (static_cast<quint8>(frame.at(2)) != kVersion)
        return std::nullopt;

    const int payloadSize = readUint16BE(frame, 4);
    if (payloadSize > kMaxPayloadSize)
        return std::nullopt;

    if (frame.size() != kMinFrameSize + payloadSize)
        return std::nullopt;

    const quint16 expected = crc16(frame.left(kHeaderSize + payloadSize));
    const quint16 actual   = readUint16BE(frame, kHeaderSize + payloadSize);
    if (expected != actual)
        return std::nullopt;

    Frame result;
    result.type    = static_cast<MessageType>(static_cast<quint8>(frame.at(3)));
    result.payload = frame.mid(kHeaderSize, payloadSize);
    return result;
}

QByteArray encodeSample(const Sample& sample)
{
    QByteArray payload;

    QDataStream stream(&payload, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

    stream << static_cast<qint32>(sample.deviceId)
           << static_cast<qint64>(sample.timestampMs)
           << sample.temperature
           << sample.pressure
           << sample.vibration;

    return payload;
}

std::optional<Sample> decodeSample(const QByteArray& payload)
{
    if (payload.size() != kSamplePayloadSize)
        return std::nullopt;

    QDataStream stream(payload);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::DoublePrecision);

    qint32 deviceId    = 0;
    qint64 timestampMs = 0;
    double temperature = 0.0;
    double pressure    = 0.0;
    double vibration   = 0.0;

    stream >> deviceId >> timestampMs >> temperature >> pressure >> vibration;

    if (stream.status() != QDataStream::Ok)
        return std::nullopt;

    Sample sample;
    sample.deviceId    = deviceId;
    sample.timestampMs = timestampMs;
    sample.temperature = temperature;
    sample.pressure    = pressure;
    sample.vibration   = vibration;
    return sample;
}
} // namespace Protocol
