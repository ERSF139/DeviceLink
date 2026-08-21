#ifndef FRAMEPARSER_H
#define FRAMEPARSER_H

#include "protocol.h"

#include <QByteArray>

#include <optional>

class FrameParser
{
public:
    void append(const QByteArray& data);
    std::optional<Protocol::Frame> nextFrame();
    void reset();
    int bufferedBytes() const;
    int droppedBytes() const;
    int badFrameCount() const;
private:
    void discardOneByte();
    QByteArray m_buffer;
    int        m_droppedBytes  = 0;
    int        m_badFrameCount = 0;
};

#endif // FRAMEPARSER_H
