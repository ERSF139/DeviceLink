#ifndef DEVICEMODEL_H
#define DEVICEMODEL_H

#include "sample.h"
#include "alarm.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QList>

class DeviceModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column
    {
        ColumnDeviceId = 0,
        ColumnTemperature,
        ColumnPressure,
        ColumnVibration,
        ColumnLastUpdate,
        ColumnSampleCount,
        ColumnAlarm,
        ColumnCount
    };

    explicit DeviceModel(QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int      columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int deviceCount() const;

    QList<int>    deviceIds() const;
    QList<Sample> historyOf(int deviceId) const;

    void setThresholds(const Alarm::Thresholds& thresholds);

public slots:
    void updateSample(const Sample& sample);
    void clear();

signals:
    void deviceAdded(int deviceId);
    void sampleAppended(int deviceId, const Sample& sample);
    void alarmChanged(int deviceId, Alarm::Level level, const QString& reason);

private:
    struct DeviceRow
    {
        int    deviceId    = 0;
        Sample latest;
        int    sampleCount = 0;
        QList<Sample> history;
        Alarm::Result alarm;
    };

    QList<DeviceRow>  m_rows;         // 行号 -> 设备数据
    QHash<int, int>   m_rowOfDevice;  // 设备ID -> 行号
    Alarm::Thresholds m_thresholds;
};

#endif // DEVICEMODEL_H