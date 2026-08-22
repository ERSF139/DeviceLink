#include "mainwindow.h"

#include "deviceclient.h"
#include "devicemodel.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableView>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
    , m_client(new DeviceClient(this))
    , m_model(new DeviceModel(this))
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
    connect(m_client,&DeviceClient::sampleReceived,
            m_model,&DeviceModel::updateSample);
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

    m_deviceView = new QTableView(this);
    m_deviceView->setModel(m_model);
    m_deviceView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceView->verticalHeader()->setVisible(false);
    m_deviceView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    m_countLabel = new QLabel("0", this);

    auto* countForm = new QFormLayout;
    countForm->addRow("累计接收帧数：", m_countLabel);

    auto* dataLayout = new QVBoxLayout;
    dataLayout->addWidget(m_deviceView, 1);
    dataLayout->addLayout(countForm);

    auto* dataGroup = new QGroupBox("实时数据", this);
    dataGroup->setLayout(dataLayout);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(connGroup);
    layout->addWidget(dataGroup, 2);      // 表格占大头
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
    if (connected) {
        m_model->clear();
        m_sampleCount = 0;
        m_countLabel->setText("0");
    }

    m_stateLabel->setText(connected ? "已连接" : "未连接");
    m_connectButton->setText(connected ? "断开" : "连接");
    m_hostEdit->setEnabled(!connected);
    m_portSpinBox->setEnabled(!connected);
}

void MainWindow::onSampleReceived(const Sample&)
{
    ++m_sampleCount;
    m_countLabel->setText(QString::number(m_sampleCount));
}

void MainWindow::appendLog(const QString& text)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logEdit->appendPlainText(QString("[%1] %2").arg(time, text));
}