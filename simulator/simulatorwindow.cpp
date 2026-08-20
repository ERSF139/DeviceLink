#include "simulatorwindow.h"

#include "device.h"
#include "deviceserver.h"

#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

SimulatorWindow::SimulatorWindow(QWidget* parent)
    : QWidget(parent)
    , m_device(new Device(1, "1号温度传感器", this))
    , m_server(new DeviceServer(this))
{
    setWindowTitle("DeviceLink 设备模拟器");
    resize(520, 620);

    buildUi();

    connect(m_device, &Device::sampleGenerated,
            this, &SimulatorWindow::onSampleGenerated);
    connect(m_device, &Device::sampleGenerated,
            m_server, &DeviceServer::broadcastSample);

    connect(m_server, &DeviceServer::logMessage,
            this, &SimulatorWindow::appendLog);
    connect(m_server, &DeviceServer::clientCountChanged,
            this, &SimulatorWindow::onClientCountChanged);

    connect(m_toggleButton, &QPushButton::clicked,
            this, &SimulatorWindow::onToggleClicked);
    connect(m_listenButton, &QPushButton::clicked,
            this, &SimulatorWindow::onListenClicked);
}

void SimulatorWindow::buildUi()
{
    m_nameLabel        = new QLabel(m_device->name(), this);
    m_statusLabel      = new QLabel("已停止", this);
    m_temperatureLabel = new QLabel("--", this);
    m_pressureLabel    = new QLabel("--", this);
    m_vibrationLabel   = new QLabel("--", this);
    m_timestampLabel   = new QLabel("--", this);
    m_toggleButton     = new QPushButton("启动", this);

    auto* deviceForm = new QFormLayout;
    deviceForm->addRow("设备名称：",    m_nameLabel);
    deviceForm->addRow("运行状态：",    m_statusLabel);
    deviceForm->addRow("温度 (°C)：",   m_temperatureLabel);
    deviceForm->addRow("压力 (kPa)：",  m_pressureLabel);
    deviceForm->addRow("振动 (mm/s)：", m_vibrationLabel);
    deviceForm->addRow("更新时间：",    m_timestampLabel);
    deviceForm->addRow(m_toggleButton);

    auto* deviceGroup = new QGroupBox("设备状态", this);
    deviceGroup->setLayout(deviceForm);

    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(1024, 65535);
    m_portSpinBox->setValue(9000);

    m_clientCountLabel = new QLabel("0", this);
    m_listenButton     = new QPushButton("开始监听", this);

    auto* serverForm = new QFormLayout;
    serverForm->addRow("监听端口：",     m_portSpinBox);
    serverForm->addRow("已连接客户端：", m_clientCountLabel);
    serverForm->addRow(m_listenButton);

    auto* serverGroup = new QGroupBox("网络服务", this);
    serverGroup->setLayout(serverForm);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(deviceGroup);
    layout->addWidget(serverGroup);
    layout->addWidget(m_logEdit, 1);
}

void SimulatorWindow::onSampleGenerated(const Sample& sample)
{
    m_temperatureLabel->setText(QString::number(sample.temperature, 'f', 2));
    m_pressureLabel->setText(QString::number(sample.pressure, 'f', 2));
    m_vibrationLabel->setText(QString::number(sample.vibration, 'f', 3));

    const QDateTime time = QDateTime::fromMSecsSinceEpoch(sample.timestampMs);
    m_timestampLabel->setText(time.toString("HH:mm:ss"));
}

void SimulatorWindow::onToggleClicked()
{
    if (m_device->isRunning()) {
        m_device->stop();
        m_statusLabel->setText("已停止");
        m_toggleButton->setText("启动");
    } else {
        m_device->start();
        m_statusLabel->setText("运行中");
        m_toggleButton->setText("停止");
    }
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

void SimulatorWindow::onClientCountChanged(int count)
{
    m_clientCountLabel->setText(QString::number(count));
}

void SimulatorWindow::appendLog(const QString& text)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logEdit->appendPlainText(QString("[%1] %2").arg(time, text));
}