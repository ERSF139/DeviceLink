#include "alarm.h"

#include <QTest>

namespace {

Sample normalSample()
{
    Sample sample;
    sample.deviceId    = 1;
    sample.timestampMs = 1700000000000LL;
    sample.temperature = 25.00;
    sample.pressure    = 101.30;
    sample.vibration   = 0.50;
    return sample;
}

} // namespace

class TestAlarm : public QObject
{
    Q_OBJECT

private slots:
    void evaluate_allNormal_hasEmptyReason();
    void evaluate_temperatureEqualsWarn_isWarning();
    void evaluate_temperatureEqualsCritical_isCritical();
    void evaluate_temperatureJustBelowWarn_isNormal();
    void evaluate_warningPlusCritical_isCritical();
    void evaluate_onlyVibrationOver_reasonMentionsVibration();
    void evaluate_pressureOverCritical_reasonMentionsPressure();
    void evaluate_allThreeOver_reasonHasThreeSegments();
    void evaluate_customThresholds_triggersAlarm();
    void levelName_allLevels_areNonEmpty();
};

void TestAlarm::evaluate_allNormal_hasEmptyReason()
{
    const Alarm::Result result = Alarm::evaluate(normalSample(), Alarm::Thresholds{});

    QCOMPARE(result.level, Alarm::Level::Normal);
    QVERIFY(result.reason.isEmpty());
}

void TestAlarm::evaluate_temperatureEqualsWarn_isWarning()
{
    Sample sample = normalSample();
    sample.temperature = 60.0;   // == warn，按 >= 算 Warning

    const Alarm::Result result = Alarm::evaluate(sample, Alarm::Thresholds{});

    QCOMPARE(result.level, Alarm::Level::Warning);
    QVERIFY(result.reason.contains(QStringLiteral("温度")));
    QVERIFY(result.reason.contains(QStringLiteral("预警")));
}

void TestAlarm::evaluate_temperatureEqualsCritical_isCritical()
{
    Sample sample = normalSample();
    sample.temperature = 85.0;   // == critical，按 >= 算 Critical

    const Alarm::Result result = Alarm::evaluate(sample, Alarm::Thresholds{});

    QCOMPARE(result.level, Alarm::Level::Critical);
    QVERIFY(result.reason.contains(QStringLiteral("温度")));
    QVERIFY(result.reason.contains(QStringLiteral("严重")));
}

void TestAlarm::evaluate_temperatureJustBelowWarn_isNormal()
{
    Sample sample = normalSample();
    sample.temperature = 59.99;

    const Alarm::Result result = Alarm::evaluate(sample, Alarm::Thresholds{});

    QCOMPARE(result.level, Alarm::Level::Normal);
    QVERIFY(result.reason.isEmpty());
}

void TestAlarm::evaluate_warningPlusCritical_isCritical()
{
    Sample sample = normalSample();
    sample.temperature = 70.0;   // Warning
    sample.vibration   = 4.5;    // Critical

    const Alarm::Result result = Alarm::evaluate(sample, Alarm::Thresholds{});

    QCOMPARE(result.level, Alarm::Level::Critical);
    QVERIFY(result.reason.contains(QStringLiteral("温度")));
    QVERIFY(result.reason.contains(QStringLiteral("振动")));
}

void TestAlarm::evaluate_onlyVibrationOver_reasonMentionsVibration()
{
    Sample sample = normalSample();
    sample.vibration = 2.80;     // >= 2.5 且 < 4.0 → Warning

    const Alarm::Result result = Alarm::evaluate(sample, Alarm::Thresholds{});

    QCOMPARE(result.level, Alarm::Level::Warning);
    QVERIFY(result.reason.contains(QStringLiteral("振动")));
    QVERIFY(!result.reason.contains(QStringLiteral("温度")));
    QVERIFY(!result.reason.contains(QStringLiteral("压力")));
}

void TestAlarm::evaluate_pressureOverCritical_reasonMentionsPressure()
{
    Sample sample = normalSample();
    sample.pressure = 126.0;     // >= 125.0 → Critical；温度/振动仍正常

    const Alarm::Result result = Alarm::evaluate(sample, Alarm::Thresholds{});

    QCOMPARE(result.level, Alarm::Level::Critical);
    QVERIFY(result.reason.contains(QStringLiteral("压力")));
    QVERIFY(!result.reason.contains(QStringLiteral("温度")));
    QVERIFY(!result.reason.contains(QStringLiteral("振动")));
}

void TestAlarm::evaluate_allThreeOver_reasonHasThreeSegments()
{
    Sample sample = normalSample();
    sample.temperature = 90.0;   // Critical
    sample.pressure    = 126.0;  // Critical
    sample.vibration   = 5.0;    // Critical

    const Alarm::Result result = Alarm::evaluate(sample, Alarm::Thresholds{});

    QCOMPARE(result.level, Alarm::Level::Critical);
    QVERIFY(result.reason.contains(QStringLiteral("温度")));
    QVERIFY(result.reason.contains(QStringLiteral("压力")));
    QVERIFY(result.reason.contains(QStringLiteral("振动")));

    const QStringList parts = result.reason.split(QStringLiteral("; "));
    QCOMPARE(parts.size(), 3);
}

void TestAlarm::evaluate_customThresholds_triggersAlarm()
{
    Sample sample = normalSample();          // 温度 25，默认阈值下是 Normal
    Alarm::Thresholds thresholds;
    thresholds.temperatureWarn = 30.0;       // 25 < 30，仍正常
    sample.temperature = 40.0;               // 默认 60 以下算正常，自定义 30 则报警

    const Alarm::Result result = Alarm::evaluate(sample, thresholds);

    QCOMPARE(result.level, Alarm::Level::Warning);
    QVERIFY(result.reason.contains(QStringLiteral("温度")));
}

void TestAlarm::levelName_allLevels_areNonEmpty()
{
    QVERIFY(!Alarm::levelName(Alarm::Level::Normal).isEmpty());
    QVERIFY(!Alarm::levelName(Alarm::Level::Warning).isEmpty());
    QVERIFY(!Alarm::levelName(Alarm::Level::Critical).isEmpty());
}

QTEST_APPLESS_MAIN(TestAlarm)

#include "tst_alarm.moc"