#include "mainwindow.h"

#include "deviceclient.h"

#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
    , m_client(new DeviceClient(this))
{
    setWindowTitle("DeviceLink 监控客户端");
    resize(560, 680);

    buildUi();

    connect(m_client, &DeviceClient::sampleReceived,
            this, &MainWindow::onSampleReceived);
    connect(m_client, &DeviceClient::connectedChanged,
            this, &MainWindow::onConnectedChanged);
    connect(m_client, &DeviceClient::logMessage,
            this, &MainWindow::appendLog);

    connect(m_connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);
}

void MainWindow::buildUi()
{
    m_hostEdit = new QLineEdit("127.0.0.1", this);

    m_portSpinBox = new QSpinBox(this);
    m_portSpinBox->setRange(1024, 65535);
    m_portSpinBox->setValue(9000);

    m_connectButton = new QPushButton("连接", this);
    m_stateLabel    = new QLabel("未连接", this);

    auto* connForm = new QFormLayout;
    connForm->addRow("服务器地址：", m_hostEdit);
    connForm->addRow("端口：",       m_portSpinBox);
    connForm->addRow("连接状态：",   m_stateLabel);
    connForm->addRow(m_connectButton);

    auto* connGroup = new QGroupBox("连接设置", this);
    connGroup->setLayout(connForm);

    m_deviceIdLabel    = new QLabel("--", this);
    m_temperatureLabel = new QLabel("--", this);
    m_pressureLabel    = new QLabel("--", this);
    m_vibrationLabel   = new QLabel("--", this);
    m_timestampLabel   = new QLabel("--", this);
    m_countLabel       = new QLabel("0", this);

    auto* dataForm = new QFormLayout;
    dataForm->addRow("设备编号：",    m_deviceIdLabel);
    dataForm->addRow("温度 (°C)：",   m_temperatureLabel);
    dataForm->addRow("压力 (kPa)：",  m_pressureLabel);
    dataForm->addRow("振动 (mm/s)：", m_vibrationLabel);
    dataForm->addRow("采样时间：",    m_timestampLabel);
    dataForm->addRow("累计接收：",    m_countLabel);

    auto* dataGroup = new QGroupBox("实时数据", this);
    dataGroup->setLayout(dataForm);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(connGroup);
    layout->addWidget(dataGroup);
    layout->addWidget(m_logEdit, 1);
}

void MainWindow::onConnectClicked()
{
    if (m_client->isConnected()) {
        m_client->disconnectFromServer();
    } else {
        m_client->connectToServer(m_hostEdit->text().trimmed(),
                                  static_cast<quint16>(m_portSpinBox->value()));
    }
}

void MainWindow::onConnectedChanged(bool connected)
{
    m_stateLabel->setText(connected ? "已连接" : "未连接");
    m_connectButton->setText(connected ? "断开" : "连接");
    m_hostEdit->setEnabled(!connected);
    m_portSpinBox->setEnabled(!connected);
}

void MainWindow::onSampleReceived(const Sample& sample)
{
    ++m_sampleCount;

    m_deviceIdLabel->setText(QString::number(sample.deviceId));
    m_temperatureLabel->setText(QString::number(sample.temperature, 'f', 2));
    m_pressureLabel->setText(QString::number(sample.pressure, 'f', 2));
    m_vibrationLabel->setText(QString::number(sample.vibration, 'f', 3));
    m_countLabel->setText(QString::number(m_sampleCount));

    const QDateTime time = QDateTime::fromMSecsSinceEpoch(sample.timestampMs);
    m_timestampLabel->setText(time.toString("HH:mm:ss.zzz"));
}

void MainWindow::appendLog(const QString& text)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logEdit->appendPlainText(QString("[%1] %2").arg(time, text));
}