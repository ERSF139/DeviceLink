#include "protocol.h"

#include <QTest>

class TestProtocol : public QObject
{
    Q_OBJECT

private slots:
    void crc16_matchesKnownVector();
    void buildFrame_hasCorrectLayout();
    void roundTrip_preservesSample();
    void parseFrame_rejectsBadHeader();
    void parseFrame_rejectsWrongVersion();
    void parseFrame_rejectsCorruptedPayload();
    void parseFrame_rejectsTruncatedFrame();
    void decodeSample_rejectsWrongPayloadSize();
    void roundTrip_emptyHeartbeat();
};

void TestProtocol::crc16_matchesKnownVector()
{
    // CRC-16/MODBUS 对 "123456789" 的标准校验值是 0x4B37
    QCOMPARE(Protocol::crc16(QByteArray("123456789")), quint16(0x4B37));
}

void TestProtocol::buildFrame_hasCorrectLayout()
{
    const QByteArray payload("ABC");
    const QByteArray frame = Protocol::buildFrame(Protocol::MessageType::Sample, payload);

    QCOMPARE(frame.size(), Protocol::kMinFrameSize + payload.size());
    QCOMPARE(static_cast<quint8>(frame.at(0)), quint8(0xA5));
    QCOMPARE(static_cast<quint8>(frame.at(1)), quint8(0x5A));
    QCOMPARE(static_cast<quint8>(frame.at(2)), quint8(0x01));   // 版本
    QCOMPARE(static_cast<quint8>(frame.at(3)), quint8(0x01));   // 类型 Sample
    QCOMPARE(static_cast<quint8>(frame.at(4)), quint8(0x00));   // 长度高字节
    QCOMPARE(static_cast<quint8>(frame.at(5)), quint8(0x03));   // 长度低字节
}

void TestProtocol::roundTrip_preservesSample()
{
    Sample original;
    original.deviceId    = 7;
    original.timestampMs = 1755500000123LL;
    original.temperature = 25.31;
    original.pressure    = 101.28;
    original.vibration   = 0.512;

    const QByteArray frame =
        Protocol::buildFrame(Protocol::MessageType::Sample, Protocol::encodeSample(original));

    const auto parsed = Protocol::parseFrame(frame);
    QVERIFY(parsed.has_value());
    QVERIFY(parsed->type == Protocol::MessageType::Sample);

    const auto decoded = Protocol::decodeSample(parsed->payload);
    QVERIFY(decoded.has_value());

    QCOMPARE(decoded->deviceId,    original.deviceId);
    QCOMPARE(decoded->timestampMs, original.timestampMs);
    QCOMPARE(decoded->temperature, original.temperature);
    QCOMPARE(decoded->pressure,    original.pressure);
    QCOMPARE(decoded->vibration,   original.vibration);
}

void TestProtocol::parseFrame_rejectsBadHeader()
{
    QByteArray frame = Protocol::buildFrame(Protocol::MessageType::Heartbeat, QByteArray());
    frame[0] = static_cast<char>(0x00);

    QVERIFY(!Protocol::parseFrame(frame).has_value());
}

void TestProtocol::parseFrame_rejectsWrongVersion()
{
    QByteArray frame = Protocol::buildFrame(Protocol::MessageType::Heartbeat, QByteArray());
    frame[2] = static_cast<char>(0x99);

    QVERIFY(!Protocol::parseFrame(frame).has_value());
}

void TestProtocol::parseFrame_rejectsCorruptedPayload()
{
    QByteArray frame =
        Protocol::buildFrame(Protocol::MessageType::Sample, Protocol::encodeSample(Sample{}));

    // 翻转载荷里的一个 bit，模拟传输过程中的数据损坏
    const int pos = Protocol::kHeaderSize + 2;
    frame[pos] = static_cast<char>(frame.at(pos) ^ 0x01);

    QVERIFY(!Protocol::parseFrame(frame).has_value());
}

void TestProtocol::parseFrame_rejectsTruncatedFrame()
{
    QByteArray frame =
        Protocol::buildFrame(Protocol::MessageType::Sample, Protocol::encodeSample(Sample{}));
    frame.chop(1);

    QVERIFY(!Protocol::parseFrame(frame).has_value());
}

void TestProtocol::decodeSample_rejectsWrongPayloadSize()
{
    QVERIFY(!Protocol::decodeSample(QByteArray()).has_value());
    QVERIFY(!Protocol::decodeSample(QByteArray(10, '\0')).has_value());
    QVERIFY(!Protocol::decodeSample(QByteArray(100, '\0')).has_value());
}

void TestProtocol::roundTrip_emptyHeartbeat()
{
    const QByteArray frame = Protocol::buildFrame(Protocol::MessageType::Heartbeat, QByteArray());

    QCOMPARE(frame.size(), Protocol::kMinFrameSize);

    const auto parsed = Protocol::parseFrame(frame);
    QVERIFY(parsed.has_value());
    QVERIFY(parsed->type == Protocol::MessageType::Heartbeat);
    QVERIFY(parsed->payload.isEmpty());
}

QTEST_APPLESS_MAIN(TestProtocol)

#include "tst_protocol.moc"