#ifndef ALARM_H
#define ALARM_H

#include "sample.h"

#include <QString>

namespace Alarm {

enum class Level
{
    Normal   = 0,
    Warning  = 1,
    Critical = 2,
};

struct Thresholds
{
    double temperatureWarn     = 60.0;
    double temperatureCritical = 85.0;
    double pressureWarn        = 115.0;
    double pressureCritical    = 125.0;
    double vibrationWarn       = 2.5;
    double vibrationCritical   = 4.0;
};

struct Result
{
    Level   level = Level::Normal;
    QString reason;      // Normal 时为空字符串
};

Result  evaluate(const Sample& sample, const Thresholds& thresholds);
QString levelName(Level level);

} // namespace Alarm

#endif // ALARM_H