#include "chartpanel.h"

#include "devicemodel.h"

#include <QChart>
#include <QChartView>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeAxis>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineSeries>
#include <QValueAxis>
#include <QVBoxLayout>

namespace {

constexpr int kVisibleSeconds  = 60;  //横轴显示最近 60 秒
constexpr int kMaxPointsPerLine = 300;//每条线最多保留的点数

} // namespace

ChartPanel::ChartPanel(DeviceModel* model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    buildUi();

    connect(m_model, &DeviceModel::deviceAdded,
            this, &ChartPanel::onDeviceAdded);
    connect(m_model, &DeviceModel::sampleAppended,
            this, &ChartPanel::onSampleAppended);
    connect(m_model, &DeviceModel::modelReset,
            this, &ChartPanel::onModelReset);

    connect(m_metricBox, &QComboBox::currentIndexChanged,
            this, &ChartPanel::onMetricChanged);
}

void ChartPanel::buildUi()
{
    m_metricBox = new QComboBox(this);
    m_metricBox->addItem("温度 (°C)",   MetricTemperature);
    m_metricBox->addItem("压力 (kPa)",  MetricPressure);
    m_metricBox->addItem("振动 (mm/s)", MetricVibration);

    auto* topRow = new QHBoxLayout;
    topRow->addWidget(new QLabel("显示指标：", this));
    topRow->addWidget(m_metricBox);
    topRow->addStretch();

    m_chart = new QChart;
    m_chart->setAnimationOptions(QChart::NoAnimation);
    m_chart->legend()->setAlignment(Qt::AlignRight);

    m_axisX = new QDateTimeAxis;
    m_axisX->setFormat("hh:mm:ss");
    m_axisX->setTickCount(5);
    m_axisX->setTitleText("时间");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);

    m_axisY = new QValueAxis;
    m_chart->addAxis(m_axisY, Qt::AlignLeft);

    applyAxisRange();

    m_chartView = new QChartView(m_chart, this);
    m_chartView->setRenderHint(QPainter::Antialiasing);

    auto* layout = new QVBoxLayout(this);
    layout->addLayout(topRow);
    layout->addWidget(m_chartView, 1);
}

ChartPanel::Metric ChartPanel::currentMetric() const
{
    return static_cast<Metric>(m_metricBox->currentData().toInt());
}

double ChartPanel::metricValue(const Sample& sample) const
{
    switch (currentMetric()) {
    case MetricTemperature: return sample.temperature;
    case MetricPressure:    return sample.pressure;
    case MetricVibration:   return sample.vibration;
    }
    return 0.0;
}

void ChartPanel::applyAxisRange()
{
    switch (currentMetric()) {
    case MetricTemperature:
        m_axisY->setRange(-20.0, 120.0);
        m_axisY->setTitleText("温度 (°C)");
        break;
    case MetricPressure:
        m_axisY->setRange(80.0, 130.0);
        m_axisY->setTitleText("压力 (kPa)");
        break;
    case MetricVibration:
        m_axisY->setRange(0.0, 5.0);
        m_axisY->setTitleText("振动 (mm/s)");
        break;
    }
}

void ChartPanel::onDeviceAdded(int deviceId)
{
    if (m_seriesOfDevice.contains(deviceId))
        return;

    auto* series = new QLineSeries(m_chart);
    series->setName(QString("设备 %1").arg(deviceId));

    m_chart->addSeries(series);
    series->attachAxis(m_axisX);
    series->attachAxis(m_axisY);

    m_seriesOfDevice.insert(deviceId, series);
}

void ChartPanel::onSampleAppended(int deviceId, const Sample& sample)
{
    QLineSeries* series = m_seriesOfDevice.value(deviceId, nullptr);
    if (!series)
        return;

    appendPoint(series, sample);

    // 横轴跟着最新时间滚动
    const QDateTime now = QDateTime::fromMSecsSinceEpoch(sample.timestampMs);
    m_axisX->setRange(now.addSecs(-kVisibleSeconds), now);
}

void ChartPanel::appendPoint(QLineSeries* series, const Sample& sample)
{
    series->append(static_cast<qreal>(sample.timestampMs), metricValue(sample));

    if (series->count() > kMaxPointsPerLine)
        series->removePoints(0, series->count() - kMaxPointsPerLine);
}

void ChartPanel::onMetricChanged()
{
    applyAxisRange();
    rebuildAllSeries();
}

void ChartPanel::rebuildAllSeries()
{
    for (auto it = m_seriesOfDevice.cbegin(); it != m_seriesOfDevice.cend(); ++it) {
        const QList<Sample> history = m_model->historyOf(it.key());
        QList<QPointF> points;
        points.reserve(history.size());
        for (const Sample& sample : history)
            points.append(QPointF(static_cast<qreal>(sample.timestampMs), metricValue(sample)));
        it.value()->replace(points);      // 一次性替换，只触发一次重绘
    }
}

void ChartPanel::onModelReset()
{
    for (QLineSeries* series : m_seriesOfDevice)
        m_chart->removeSeries(series);

    qDeleteAll(m_seriesOfDevice);
    m_seriesOfDevice.clear();
}