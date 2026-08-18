#include "device.h"

#include <QDateTime>
#include <QRandomGenerator>
#include <QTimer>

namespace {
constexpr int kSampleIntervalMs = 1000;

double randomWalk(double current,double maxStep,double lowerBound,double upperBound)
{
    const double delta = QRandomGenerator::global()->bounded(2.0 * maxStep)-maxStep;
    return qBound(lowerBound,current+delta,upperBound);
}
}//namespace

Device::Device(int id, const QString& name, QObject* parent)
    : QObject(parent)
    , m_id(id)
    , m_name(name)
    , m_timer(new QTimer(this))
    , m_temperature(25.0)
    , m_pressure(101.3)
    , m_vibration(0.5)
{    m_timer->setInterval(kSampleIntervalMs);
    connect(m_timer, &QTimer::timeout, this, &Device::generateSample);
}

int Device::id() const
{
    return m_id;
}
QString Device::name() const
{
    return m_name;
}
bool Device::isRunning() const
{
    return m_timer->isActive();
}
void Device::start()
{
    m_timer->start();
}
void Device::stop()
{
    m_timer->stop();
}
void Device::generateSample()
{
    m_temperature = randomWalk(m_temperature, 0.4,  -20.0, 120.0);
    m_pressure    = randomWalk(m_pressure,    0.3,   80.0, 130.0);
    m_vibration   = randomWalk(m_vibration,   0.05,   0.0,   5.0);
    Sample sample;
    sample.deviceId    = m_id;
    sample.timestampMs = QDateTime::currentMSecsSinceEpoch();
    sample.temperature = m_temperature;
    sample.pressure    = m_pressure;
    sample.vibration   = m_vibration;
    emit sampleGenerated(sample);
}