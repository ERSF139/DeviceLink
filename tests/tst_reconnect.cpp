#include "reconnect.h"

#include <QTest>

class TestReconnect : public QObject
{
    Q_OBJECT

private slots:
    void nextDelay_startsAtInitial();
    void nextDelay_doublesUntilCap();
    void watchdog_coversThreeHeartbeats();
};

void TestReconnect::nextDelay_startsAtInitial()
{
    QCOMPARE(Reconnect::nextDelayMs(0), Reconnect::kInitialDelayMs);
    QCOMPARE(Reconnect::nextDelayMs(500), Reconnect::kInitialDelayMs);
}

void TestReconnect::nextDelay_doublesUntilCap()
{
    QCOMPARE(Reconnect::nextDelayMs(1000), 2000);
    QCOMPARE(Reconnect::nextDelayMs(2000), 4000);
    QCOMPARE(Reconnect::nextDelayMs(4000), 8000);
    QCOMPARE(Reconnect::nextDelayMs(8000), 16000);
    QCOMPARE(Reconnect::nextDelayMs(16000), 16000);
    QCOMPARE(Reconnect::nextDelayMs(32000), Reconnect::kMaxDelayMs);
}

void TestReconnect::watchdog_coversThreeHeartbeats()
{
    QVERIFY(Reconnect::kWatchdogTimeoutMs >= 3 * Reconnect::kHeartbeatIntervalMs);
}

QTEST_APPLESS_MAIN(TestReconnect)

#include "tst_reconnect.moc"
