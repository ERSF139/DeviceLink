#include "frameparser.h"
#include "protocol.h"

#include <QTest>

namespace {

QByteArray makeSampleFrame(int deviceId)
{
    Sample sample;
    sample.deviceId    = deviceId;
    sample.timestampMs = 1700000000000LL + deviceId;
    sample.temperature = 20.0 + deviceId;
    sample.pressure    = 100.0 + deviceId;
    sample.vibration   = 0.5;

    return Protocol::buildFrame(Protocol::MessageType::Sample,
                                Protocol::encodeSample(sample));
}

} // namespace

class TestFrameParser : public QObject
{
    Q_OBJECT

private slots:
    void singleFrame();
    void multipleFramesInOneChunk();
    void frameSplitAcrossChunks();
    void byteByByte();
    void skipsLeadingGarbage();
    void recoversAfterCorruptedFrame();
    void rejectsOversizedLength();
    void rejectsLengthNotMatchingTypeWithoutWaiting();
    void rejectsUnknownTypeAtHeader();
    void emptyInputYieldsNothing();
};

void TestFrameParser::singleFrame()
{
    FrameParser parser;
    parser.append(makeSampleFrame(1));

    const auto frame = parser.nextFrame();
    QVERIFY(frame.has_value());
    QVERIFY(frame->type == Protocol::MessageType::Sample);

    const auto sample = Protocol::decodeSample(frame->payload);
    QVERIFY(sample.has_value());
    QCOMPARE(sample->deviceId, 1);

    QVERIFY(!parser.nextFrame().has_value());
    QCOMPARE(parser.bufferedBytes(), 0);
    QCOMPARE(parser.droppedBytes(), 0);
}

void TestFrameParser::multipleFramesInOneChunk()
{
    QByteArray chunk;
    for (int i = 1; i <= 5; ++i)
        chunk.append(makeSampleFrame(i));

    FrameParser parser;
    parser.append(chunk);                     // 五帧一次性喂进去，模拟粘包

    for (int i = 1; i <= 5; ++i) {
        const auto frame = parser.nextFrame();
        QVERIFY(frame.has_value());

        const auto sample = Protocol::decodeSample(frame->payload);
        QVERIFY(sample.has_value());
        QCOMPARE(sample->deviceId, i);
    }

    QVERIFY(!parser.nextFrame().has_value());
    QCOMPARE(parser.droppedBytes(), 0);
}

void TestFrameParser::frameSplitAcrossChunks()
{
    const QByteArray frame = makeSampleFrame(9);
    FrameParser parser;

    parser.append(frame.left(3));             // 连头部都不完整
    QVERIFY(!parser.nextFrame().has_value());

    parser.append(frame.mid(3, 20));          // 头部够了，载荷没齐
    QVERIFY(!parser.nextFrame().has_value());

    parser.append(frame.mid(23));             // 补齐
    const auto parsed = parser.nextFrame();
    QVERIFY(parsed.has_value());

    const auto sample = Protocol::decodeSample(parsed->payload);
    QVERIFY(sample.has_value());
    QCOMPARE(sample->deviceId, 9);
}

void TestFrameParser::byteByByte()
{
    QByteArray stream;
    for (int i = 1; i <= 3; ++i)
        stream.append(makeSampleFrame(i));

    FrameParser parser;
    QList<int> received;

    for (int i = 0; i < stream.size(); ++i) {
        parser.append(stream.mid(i, 1));      // 一次只喂 1 个字节

        while (const auto frame = parser.nextFrame()) {
            const auto sample = Protocol::decodeSample(frame->payload);
            QVERIFY(sample.has_value());
            received.append(sample->deviceId);
        }
    }

    QCOMPARE(received.size(), 3);
    QCOMPARE(received.at(0), 1);
    QCOMPARE(received.at(1), 2);
    QCOMPARE(received.at(2), 3);
    QCOMPARE(parser.droppedBytes(), 0);
}

void TestFrameParser::skipsLeadingGarbage()
{
    FrameParser parser;
    parser.append(QByteArray("garbage before the real frame"));
    parser.append(makeSampleFrame(4));

    const auto frame = parser.nextFrame();
    QVERIFY(frame.has_value());

    const auto sample = Protocol::decodeSample(frame->payload);
    QVERIFY(sample.has_value());
    QCOMPARE(sample->deviceId, 4);
    QVERIFY(parser.droppedBytes() > 0);
}

void TestFrameParser::recoversAfterCorruptedFrame()
{
    QByteArray bad = makeSampleFrame(1);
    const int pos  = Protocol::kHeaderSize + 3;
    bad[pos] = static_cast<char>(bad.at(pos) ^ 0xFF);      // 改坏载荷，CRC 必然对不上

    FrameParser parser;
    parser.append(bad);
    parser.append(makeSampleFrame(2));

    const auto frame = parser.nextFrame();
    QVERIFY(frame.has_value());

    const auto sample = Protocol::decodeSample(frame->payload);
    QVERIFY(sample.has_value());
    QCOMPARE(sample->deviceId, 2);                         // 坏帧被跳过，后一帧正常解析
    QVERIFY(parser.badFrameCount() >= 1);
}

void TestFrameParser::rejectsOversizedLength()
{
    QByteArray evil;
    evil.append(static_cast<char>(0xA5));
    evil.append(static_cast<char>(0x5A));
    evil.append(static_cast<char>(0x01));      // 版本
    evil.append(static_cast<char>(0x01));      // 类型
    evil.append(static_cast<char>(0xFF));      // 长度 = 0xFFFF = 65535
    evil.append(static_cast<char>(0xFF));
    evil.append(QByteArray(10, '\0'));

    FrameParser parser;
    parser.append(evil);
    parser.append(makeSampleFrame(3));

    const auto frame = parser.nextFrame();
    QVERIFY(frame.has_value());

    const auto sample = Protocol::decodeSample(frame->payload);
    QVERIFY(sample.has_value());
    QCOMPARE(sample->deviceId, 3);
}

void TestFrameParser::rejectsLengthNotMatchingTypeWithoutWaiting()
{
    // Sample 的长度字段写成 4000：修复前要攒满 4008 字节才能靠 CRC 判定，
    // 这里后面只跟了一帧 44 字节的合法帧，修复后应该立刻解析出它。
    QByteArray fake;
    fake.append(static_cast<char>(0xA5));
    fake.append(static_cast<char>(0x5A));
    fake.append(static_cast<char>(0x01));      // 版本
    fake.append(static_cast<char>(0x01));      // 类型 Sample
    fake.append(static_cast<char>(0x0F));      // 长度 = 0x0FA0 = 4000
    fake.append(static_cast<char>(0xA0));

    FrameParser parser;
    parser.append(fake);
    parser.append(makeSampleFrame(6));

    const auto frame = parser.nextFrame();
    QVERIFY(frame.has_value());

    const auto sample = Protocol::decodeSample(frame->payload);
    QVERIFY(sample.has_value());
    QCOMPARE(sample->deviceId, 6);
    QCOMPARE(parser.badFrameCount(), 1);
}

void TestFrameParser::rejectsUnknownTypeAtHeader()
{
    QByteArray unknown = Protocol::buildFrame(static_cast<Protocol::MessageType>(0x03), QByteArray());

    FrameParser parser;
    parser.append(unknown);
    parser.append(makeSampleFrame(8));

    const auto frame = parser.nextFrame();
    QVERIFY(frame.has_value());
    QVERIFY(frame->type == Protocol::MessageType::Sample);
    QVERIFY(parser.badFrameCount() >= 1);
}

void TestFrameParser::emptyInputYieldsNothing()
{
    FrameParser parser;
    QVERIFY(!parser.nextFrame().has_value());

    parser.append(QByteArray());
    QVERIFY(!parser.nextFrame().has_value());
}

QTEST_APPLESS_MAIN(TestFrameParser)

#include "tst_frameparser.moc"