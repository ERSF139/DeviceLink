#include "frameparser.h"

namespace {

// 从 from 开始搜索帧头 A5 5A，返回起始下标；找不到返回 -1
int findHeader(const QByteArray& buffer, int from)
{
    const int last = static_cast<int>(buffer.size()) - 1;   // 帧头占 2 字节，最后一个字节单独不算

    for (int i = from; i < last; ++i) {
        if (static_cast<quint8>(buffer.at(i)) == Protocol::kHeaderByte0
            && static_cast<quint8>(buffer.at(i + 1)) == Protocol::kHeaderByte1) {
            return i;
        }
    }

    return -1;
}

} // namespace

void FrameParser::append(const QByteArray& data)
{
    m_buffer.append(data);
}

void FrameParser::reset()
{
    m_buffer.clear();
}

int FrameParser::bufferedBytes() const
{
    return static_cast<int>(m_buffer.size());
}

int FrameParser::droppedBytes() const
{
    return m_droppedBytes;
}

int FrameParser::badFrameCount() const
{
    return m_badFrameCount;
}

void FrameParser::discardOneByte()
{
    m_buffer.remove(0, 1);
    ++m_droppedBytes;
}

std::optional<Protocol::Frame> FrameParser::nextFrame()
{
    while (true) {
        if (m_buffer.size() < Protocol::kMinFrameSize)
            return std::nullopt;

        const int headerPos = findHeader(m_buffer, 0);

        if (headerPos < 0) {
            // 整个缓冲区里没有帧头。保留最后 1 个字节：
            // 可能是 0xA5，0x5A 在下一批数据
            const int drop = static_cast<int>(m_buffer.size()) - 1;
            m_buffer.remove(0, drop);
            m_droppedBytes += drop;
            return std::nullopt;
        }

        if (headerPos > 0) {
            m_buffer.remove(0, headerPos);
            m_droppedBytes += headerPos;
            continue;                       // 长度可能又不够了，重新来一轮
        }

        // 缓冲区现在以帧头开头。帧头 6 字节已经到齐（上面保证至少有 kMinFrameSize 字节），
        // 先用帧头判断真假：版本不对、类型未知、长度和类型对不上，都立刻拒绝，
        // 不要为一个假帧头等上最多 4104 字节。
        const int payloadSize = Protocol::readUint16BE(m_buffer, 4);
        const auto expectedSize =
            Protocol::expectedPayloadSize(static_cast<quint8>(m_buffer.at(3)));

        if (static_cast<quint8>(m_buffer.at(2)) != Protocol::kVersion
            || !expectedSize || *expectedSize != payloadSize
            || payloadSize > Protocol::kMaxPayloadSize) {
            ++m_badFrameCount;
            discardOneByte();               // 假帧头，滑一个字节继续找
            continue;
        }

        const int frameSize = Protocol::kMinFrameSize + payloadSize;

        if (m_buffer.size() < frameSize)
            return std::nullopt;            // 数据还没到齐，原样留着等下一批

        const auto frame = Protocol::parseFrame(m_buffer.left(frameSize));

        if (!frame) {
            ++m_badFrameCount;
            discardOneByte();               // CRC 或版本错，重同步
            continue;
        }

        m_buffer.remove(0, frameSize);
        return frame;
    }
}