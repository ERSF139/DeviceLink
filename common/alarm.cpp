#include "alarm.h"

#include <QStringList>

#include <algorithm>

namespace Alarm {

namespace {

Level evaluateMetric(double value,
                     double warn,
                     double critical,
                     const QString& name,
                     QStringList* reasons)
{
    if (value >= critical) {
        reasons->append(QString("%1 %2 超过严重阈值 %3")
                            .arg(name)
                            .arg(QString::number(value, 'f', 2))
                            .arg(QString::number(critical, 'f', 2)));
        return Level::Critical;
    }

    if (value >= warn) {
        reasons->append(QString("%1 %2 超过预警阈值 %3")
                            .arg(name)
                            .arg(QString::number(value, 'f', 2))
                            .arg(QString::number(warn, 'f', 2)));
        return Level::Warning;
    }

    return Level::Normal;
}

} // namespace

Result evaluate(const Sample& sample, const Thresholds& thresholds)
{
    QStringList reasons;

    const Level temperature = evaluateMetric(
        sample.temperature,
        thresholds.temperatureWarn,
        thresholds.temperatureCritical,
        QStringLiteral("温度"),
        &reasons);
    const Level pressure = evaluateMetric(
        sample.pressure,
        thresholds.pressureWarn,
        thresholds.pressureCritical,
        QStringLiteral("压力"),
        &reasons);
    const Level vibration = evaluateMetric(
        sample.vibration,
        thresholds.vibrationWarn,
        thresholds.vibrationCritical,
        QStringLiteral("振动"),
        &reasons);

    Result result;
    result.level  = std::max({temperature, pressure, vibration});
    result.reason = reasons.join(QStringLiteral("; "));
    return result;
}

QString levelName(Level level)
{
    switch (level) {
    case Level::Normal:   return QStringLiteral("正常");
    case Level::Warning:  return QStringLiteral("预警");
    case Level::Critical: return QStringLiteral("严重");
    }
    return QStringLiteral("未知");
}

} // namespace Alarm