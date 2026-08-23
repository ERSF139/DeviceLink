#include "devicemodel.h"

#include <QDateTime>
#include <QStringList>

namespace {

constexpr int kMaxHistory = 300;  // 1 Hz 下约 5 分

const QStringList& headerLabels()
{
    static const QStringList labels{
        "设备编号", "温度(°C)", "压力(kPa)", "振动(mm/s)", "最后更新", "累计帧数"};
    return labels;
}

void appendHistory(QList<Sample>& history, const Sample& sample)
{
    history.append(sample);
    if (history.size() > kMaxHistory)
        history.removeFirst();
}

} // namespace

DeviceModel::DeviceModel(QObject *parent)
    : QAbstractTableModel{parent}
{}

int DeviceModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}
int DeviceModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return ColumnCount;
}

QVariant DeviceModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();

    if (role == Qt::TextAlignmentRole)
        return static_cast<int>(Qt::AlignCenter);

    if (role != Qt::DisplayRole)
        return QVariant();

    const DeviceRow& row = m_rows.at(index.row());

    switch (index.column()) {
    case ColumnDeviceId:    return row.deviceId;
    case ColumnTemperature: return QString::number(row.latest.temperature, 'f', 2);
    case ColumnPressure:    return QString::number(row.latest.pressure,    'f', 2);
    case ColumnVibration:   return QString::number(row.latest.vibration,   'f', 3);
    case ColumnLastUpdate:
        return QDateTime::fromMSecsSinceEpoch(row.latest.timestampMs).toString("HH:mm:ss");
    case ColumnSampleCount: return row.sampleCount;
    default:                return QVariant();
    }
}

QVariant DeviceModel::headerData(int section, Qt::Orientation orientation,int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal) {
        if (section < 0 || section >= headerLabels().size())
            return QVariant();

        return headerLabels().at(section);
    }
    return QVariant();
}

QList<int> DeviceModel::deviceIds() const
{
    QList<int> ids;
    ids.reserve(m_rows.size());
    for (const DeviceRow& row : m_rows)
        ids.append(row.deviceId);
    return ids;
}

QList<Sample> DeviceModel::historyOf(int deviceId) const
{
    const auto it = m_rowOfDevice.constFind(deviceId);
    if (it == m_rowOfDevice.cend())
        return {};
    return m_rows.at(it.value()).history;
}

int DeviceModel::deviceCount() const
{
    return m_rows.size();
}

void DeviceModel::updateSample(const Sample& sample)
{
    const auto it = m_rowOfDevice.constFind(sample.deviceId);

    if (it == m_rowOfDevice.cend()) {
        // 新设备：在末尾插入一行
        const int row = m_rows.size();

        beginInsertRows(QModelIndex(), row, row);

        DeviceRow deviceRow;
        deviceRow.deviceId    = sample.deviceId;
        deviceRow.latest      = sample;
        deviceRow.sampleCount = 1;
        appendHistory(deviceRow.history, sample);
        m_rows.append(deviceRow);
        m_rowOfDevice.insert(sample.deviceId, row);

        endInsertRows();
        emit deviceAdded(sample.deviceId);
        emit sampleAppended(sample.deviceId, sample);
        return;
    }

    // 已有设备：更新那一行
    const int row = it.value();
    DeviceRow& deviceRow = m_rows[row];
    deviceRow.latest = sample;
    ++deviceRow.sampleCount;
    appendHistory(deviceRow.history, sample);

    emit dataChanged(index(row, 0), index(row, ColumnCount - 1));
    emit sampleAppended(sample.deviceId, sample);
}

void DeviceModel::clear()
{
    beginResetModel();
    m_rows.clear();
    m_rowOfDevice.clear();
    endResetModel();
}