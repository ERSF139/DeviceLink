#include "simulatorwindow.h"

#include "device.h"
#include "deviceserver.h"
#include "gatewayuplink.h"

#include <algorithm>
#include <utility>
#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QAbstractItemView>

namespace {
constexpr int kDeviceCount = 5;
}// namespace

SimulatorWindow::SimulatorWindow(QWidget* parent)
    : QWidget(parent)
    , m_server(new DeviceServer(this))
    , m_uplink(new GatewayUplink(this))
{
    setWindowTitle("DeviceLink 设备模拟器");
    resize(760, 820);

    buildUi();
    createDevices();

    connect(m_server, &DeviceServer::logMessage,
            this, &SimulatorWindow::appendLog);
    connect(m_server, &DeviceServer::clientCountChanged,
            this, &SimulatorWindow::onClientCountChanged);
    connect(m_uplink, &GatewayUplink::logMessage,
            this, &SimulatorWindow::appendLog);
    connect(m_uplink, &GatewayUplink::connectedCountChanged, this, [this](int count) {
        m_gatewayStatusLabel->setText(QString("%1 / %2").arg(count).arg(m_devices.size()));
    });
    connect(m_gatewayButton, &QPushButton::clicked,
            this, &SimulatorWindow::onGatewayClicked);

    connect(m_toggleAllButton, &QPushButton::clicked,
            this, &SimulatorWindow::onToggleAllClicked);
    connect(m_toggleSelectedButton,&QPushButton::clicked,
            this,&SimulatorWindow::onToggleSelectedClicked);
    connect(m_injectSpikeButton, &QPushButton::clicked,
            this, &SimulatorWindow::onInjectSpikeClicked);
    connect(m_resetValuesButton, &QPushButton::clicked,
            this, &SimulatorWindow::onResetValuesClicked);
    connect(m_listenButton, &QPushButton::clicked,
            this, &SimulatorWindow::onListenClicked);
    connect(m_deviceTable, &QTableWidget::itemSelectionChanged,
            this, &SimulatorWindow::updateControls);
}

void SimulatorWindow::buildUi()
{
    m_deviceTable = new QTableWidget(kDeviceCount, 6, this);
    m_deviceTable->setHorizontalHeaderLabels(
        {"设备", "状态", "温度(°C)", "压力(kPa)", "振动(mm/s)", "更新时间"});

    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);//不可编辑
    m_deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);//选中单元格所在行
    m_deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);//单行选择，一次只允许选择一行
    m_deviceTable->verticalHeader()->setVisible(false);//隐藏表格左边的行号列
    m_deviceTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);//拉伸自适应列宽大小

    // 预先把所有格子创建出来，之后 item(row, col) 不会返回 nullptr
    for (int row = 0; row < kDeviceCount; ++row) {
        for (int col = 0; col < m_deviceTable->columnCount(); ++col) {
            auto* item = new QTableWidgetItem("--");
            item->setTextAlignment(Qt::AlignCenter);
            m_deviceTable->setItem(row, col, item);
        }
    }

    m_toggleAllButton = new QPushButton("全部启动", this);
    m_toggleSelectedButton  = new QPushButton("选择启动", this);
    m_injectSpikeButton = new QPushButton("注入异常", this);
    m_resetValuesButton = new QPushButton("恢复正常", this);

    auto* buttonRow = new QHBoxLayout;
    buttonRow->addWidget(m_toggleAllButton);
    buttonRow->addWidget(m_toggleSelectedButton);
    buttonRow->addWidget(m_injectSpikeButton);
    buttonRow->addWidget(m_resetValuesButton);

    auto* deviceLayout = new QVBoxLayout;
    deviceLayout->addWidget(m_deviceTable);
    deviceLayout->addLayout(buttonRow);

    auto* deviceGroup = new QGroupBox("设备状态", this);
    deviceGroup->setLayout(deviceLayout);

    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(1024, 65535);
    m_portSpinBox->setValue(9000);

    m_clientCountLabel = new QLabel("0", this);
    m_listenButton     = new QPushButton("开始监听", this);

    auto* serverForm = new QFormLayout;
    serverForm->addRow("监听端口：",     m_portSpinBox);
    serverForm->addRow("已连接客户端：", m_clientCountLabel);
    serverForm->addRow(m_listenButton);

    auto* serverGroup = new QGroupBox("网络服务（直连模式：客户端连到模拟器）", this);
    serverGroup->setLayout(serverForm);

    m_gatewayHostEdit = new QLineEdit("127.0.0.1", this);
    m_gatewayPortSpinBox = new QSpinBox(this);
    m_gatewayPortSpinBox->setRange(1024, 65535);
    m_gatewayPortSpinBox->setValue(9100);
    m_gatewayStatusLabel = new QLabel("未连接", this);
    m_gatewayButton      = new QPushButton("连接网关", this);

    auto* gatewayForm = new QFormLayout;
    gatewayForm->addRow("网关地址：",       m_gatewayHostEdit);
    gatewayForm->addRow("设备端口：",       m_gatewayPortSpinBox);
    gatewayForm->addRow("已连上的设备：",   m_gatewayStatusLabel);
    gatewayForm->addRow(m_gatewayButton);

    auto* gatewayGroup = new QGroupBox("LinkGate 网关（网关模式：每台设备一条连接连到网关）", this);
    gatewayGroup->setLayout(gatewayForm);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(deviceGroup);
    layout->addWidget(serverGroup);
    layout->addWidget(gatewayGroup);
    layout->addWidget(m_logEdit, 1);
}

void SimulatorWindow::createDevices()
{
    for (int i = 0; i < kDeviceCount; ++i) {
        const int id       = i + 1;
        const int interval = 800 + i * 100;
        auto* device = new Device(id, QString("设备 %1").arg(id), interval, this);
        m_devices.append(device);
        connect(device, &Device::sampleGenerated,
                this, &SimulatorWindow::onSampleGenerated);
        connect(device, &Device::sampleGenerated,
                m_server, &DeviceServer::broadcastSample);
        connect(device, &Device::sampleGenerated,
                m_uplink, &GatewayUplink::sendSample);
        m_deviceTable->item(i, 0)->setText(device->name());
        refreshStatusCell(i);
    }
    updateControls();
}

void SimulatorWindow::onSampleGenerated(const Sample& sample)
{
    const int row = sample.deviceId - 1;
    if(row < 0 || row >= kDeviceCount)
        return;

    m_deviceTable->item(row, 2)->setText(QString::number(sample.temperature, 'f', 2));
    m_deviceTable->item(row, 3)->setText(QString::number(sample.pressure,    'f', 2));
    m_deviceTable->item(row, 4)->setText(QString::number(sample.vibration,   'f', 3));

    const QDateTime time = QDateTime::fromMSecsSinceEpoch(sample.timestampMs);
    m_deviceTable->item(row,5)->setText(time.toString("HH:mm:ss"));
}

void SimulatorWindow::onToggleAllClicked()
{
    const bool anyRunning = anyDeviceRunning();

    for (Device* device : m_devices) {
        if (anyRunning)
            device->stop();
        else
            device->start();
    }

    for (int row = 0; row < m_devices.size(); ++row)
        refreshStatusCell(row);

    updateControls();
}

void SimulatorWindow::onToggleSelectedClicked()
{
    const int row = m_deviceTable->currentRow();
    if (row < 0 || row >= m_devices.size())
        return;
    Device* device = m_devices.at(row);
    if (device->isRunning())
        device->stop();
    else
        device->start();
    refreshStatusCell(row);
    updateControls();
}

void SimulatorWindow::onInjectSpikeClicked()
{
    const int row = m_deviceTable->currentRow();
    if (row < 0 || row >= m_devices.size())
        return;

    m_devices.at(row)->injectSpike();
    appendLog(QString("已向 %1 注入异常").arg(m_devices.at(row)->name()));
}

void SimulatorWindow::onResetValuesClicked()
{
    const int row = m_deviceTable->currentRow();
    if (row < 0 || row >= m_devices.size())
        return;

    m_devices.at(row)->resetValues();
    appendLog(QString("已将 %1 恢复正常").arg(m_devices.at(row)->name()));
}

void SimulatorWindow::onListenClicked()
{
    if (m_server->isListening()) {
        m_server->stopListening();
        m_portSpinBox->setEnabled(true);
        m_listenButton->setText("开始监听");
    } else {
        const quint16 port = static_cast<quint16>(m_portSpinBox->value());
        if (m_server->startListening(port)) {
            m_portSpinBox->setEnabled(false);
            m_listenButton->setText("停止监听");
        }
    }
}

void SimulatorWindow::onGatewayClicked()
{
    if (m_uplink->isActive()) {
        m_uplink->stop();
        m_gatewayHostEdit->setEnabled(true);
        m_gatewayPortSpinBox->setEnabled(true);
        m_gatewayButton->setText("连接网关");
        m_gatewayStatusLabel->setText("未连接");
        return;
    }

    QList<int> ids;
    for (const Device* device : std::as_const(m_devices))
        ids.append(device->id());

    m_uplink->start(m_gatewayHostEdit->text().trimmed(),
                    static_cast<quint16>(m_gatewayPortSpinBox->value()), ids);
    m_gatewayHostEdit->setEnabled(false);
    m_gatewayPortSpinBox->setEnabled(false);
    m_gatewayButton->setText("断开网关");
    m_gatewayStatusLabel->setText(QString("0 / %1").arg(ids.size()));
}

void SimulatorWindow::onClientCountChanged(int count)
{
    m_clientCountLabel->setText(QString::number(count));
}

void SimulatorWindow::appendLog(const QString& text)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logEdit->appendPlainText(QString("[%1] %2").arg(time, text));
}

bool SimulatorWindow::anyDeviceRunning() const
{
    return std::any_of(m_devices.cbegin(), m_devices.cend(),
                       [](const Device* device) { return device->isRunning(); });
}

void SimulatorWindow::updateControls()
{
    for (int row = 0; row < m_devices.size(); ++row)
        refreshStatusCell(row);
    m_toggleAllButton->setText(anyDeviceRunning() ? "全部停止" : "全部启动");

    const int  row          = m_deviceTable->currentRow();
    const bool hasSelection = (row >= 0 && row < m_devices.size());

    m_toggleSelectedButton->setEnabled(hasSelection);
    m_toggleSelectedButton->setText(
        hasSelection && m_devices.at(row)->isRunning() ? "停止所选" : "启动所选");
    m_injectSpikeButton->setEnabled(hasSelection);
    m_resetValuesButton->setEnabled(hasSelection);
}

void SimulatorWindow::refreshStatusCell(int row)
{
    if (row < 0 || row >= m_devices.size())
        return;
    const bool running = m_devices.at(row)->isRunning();
    m_deviceTable->item(row, 1)->setText(running ? "运行中" : "已停止");
}