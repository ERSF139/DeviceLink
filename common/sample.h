#ifndef SAMPLE_H
#define SAMPLE_H

#include <QMetaType>
#include <QtGlobal>

struct Sample
{
    int    deviceId = 0;         //设备Id
    qint64 timestampMs = 0;      //时间戳
    double temperature = 0.0;    //温度
    double pressure = 0.0;       //压力
    double vibration = 0.0;      //振动
};

Q_DECLARE_METATYPE(Sample)

#endif // SAMPLE_H
