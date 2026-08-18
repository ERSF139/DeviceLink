#include "simulatorwindow.h"

#include "device.h"

#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

SimulatorWindow::SimulatorWindow(QWidget* parent)
    : QWidget(parent)
    , m_device(new Device(1, "1号温度传感器", this))
{
    setWindowTitle("DeviceLink 设备模拟器");
    resize(480, 360);

    buildUi();

    connect(m_device, &Device::sampleGenerated,
            this, &SimulatorWindow::onSampleGenerated);
    connect(m_toggleButton, &QPushButton::clicked,
            this, &SimulatorWindow::onToggleClicked);
}

void SimulatorWindow::buildUi()
{
    m_nameLabel        = new QLabel(m_device->name(), this);
    m_statusLabel      = new QLabel("已停止", this);
    m_temperatureLabel = new QLabel("--", this);
    m_pressureLabel    = new QLabel("--", this);
    m_vibrationLabel   = new QLabel("--", this);
    m_timestampLabel   = new QLabel("--", this);

    auto* form = new QFormLayout;
    form->addRow("设备名称：",    m_nameLabel);
    form->addRow("运行状态：",    m_statusLabel);
    form->addRow("温度 (°C)：",   m_temperatureLabel);
    form->addRow("压力 (kPa)：",  m_pressureLabel);
    form->addRow("振动 (mm/s)：", m_vibrationLabel);
    form->addRow("更新时间：",    m_timestampLabel);

    auto* group = new QGroupBox("设备状态", this);
    group->setLayout(form);

    m_toggleButton = new QPushButton("启动", this);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(group);
    layout->addWidget(m_toggleButton);
    layout->addStretch();
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