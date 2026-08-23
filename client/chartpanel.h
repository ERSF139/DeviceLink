#ifndef CHARTPANEL_H
#define CHARTPANEL_H

#include "sample.h"

#include <QHash>
#include <QWidget>

class DeviceModel;
class QChart;
class QChartView;
class QComboBox;
class QDateTimeAxis;
class QLineSeries;
class QValueAxis;

class ChartPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ChartPanel(DeviceModel* model, QWidget* parent = nullptr);

private:
    enum Metric
    {
        MetricTemperature = 0,
        MetricPressure,
        MetricVibration
    };

    void buildUi();

    void onDeviceAdded(int deviceId);
    void onSampleAppended(int deviceId, const Sample& sample);
    void onMetricChanged();
    void onModelReset();

    Metric currentMetric() const;
    double metricValue(const Sample& sample) const;
    void   applyAxisRange();
    void   rebuildAllSeries();
    void   appendPoint(QLineSeries* series, const Sample& sample);

    DeviceModel* m_model;

    QChart*        m_chart;
    QChartView*    m_chartView;
    QComboBox*     m_metricBox;
    QDateTimeAxis* m_axisX;
    QValueAxis*    m_axisY;

    QHash<int, QLineSeries*> m_seriesOfDevice;
};

#endif // CHARTPANEL_H