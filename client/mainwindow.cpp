#include "mainwindow.h"

#include "chartpanel.h"
#include "deviceclient.h"
#include "devicemodel.h"
#include "sampledatabase.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QDir>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTableView>
#include <QThread>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
    , m_client(new DeviceClient(this))
    , m_model(new DeviceModel(this))
    , m_dbThread(new QThread(this))
    , m_database(new SampleDatabase)
{
    setWindowTitle("DeviceLink 监控客户端");
    resize(980, 860);

    buildUi();
    m_storageLabel->setText(QStringLiteral("启动中…"));

    connect(m_client, &DeviceClient::sampleReceived,
            this, &MainWindow::onSampleReceived);
    connect(m_client, &DeviceClient::connectedChanged,
            this, &MainWindow::onConnectedChanged);
    connect(m_client, &DeviceClient::reconnectScheduled,
            this, &MainWindow::onReconnectScheduled);
    connect(m_client, &DeviceClient::logMessage,
            this, &MainWindow::appendLog);

    connect(m_connectButton, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);
    connect(m_client,&DeviceClient::sampleReceived,
            m_model,&DeviceModel::updateSample);
    connect(m_model, &DeviceModel::alarmChanged,
            this, &MainWindow::onAlarmChanged);

    const QString dataDir =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    const QString dbPath = QDir(dataDir).filePath(QStringLiteral("devicelink.db"));

    m_database->moveToThread(m_dbThread);
    connect(m_dbThread, &QThread::started, m_database, [database = m_database, dbPath]() {
        database->initialize(dbPath);
    });
    connect(m_dbThread, &QThread::finished, m_database, &QObject::deleteLater);
    connect(m_client, &DeviceClient::sampleReceived,
            m_database, &SampleDatabase::enqueue);
    connect(m_database, &SampleDatabase::opened,
            this, &MainWindow::onOpened);
    connect(m_database, &SampleDatabase::logMessage,
            this, &MainWindow::appendLog);
    connect(m_database, &SampleDatabase::flushed,
            this, &MainWindow::onFlushed);
    m_dbThread->start();
}

MainWindow::~MainWindow()
{
    disconnect(m_database, nullptr, this, nullptr);

    if (m_dbThread->isRunning()) {
        QMetaObject::invokeMethod(m_database, &SampleDatabase::shutdown,
                                  Qt::BlockingQueuedConnection);
        m_dbThread->quit();
        m_dbThread->wait();
    }
}

void MainWindow::onOpened(qint64 totalRows)
{
    m_storageLabel->setText(QString("%1 条").arg(totalRows));
}

void MainWindow::onFlushed(int rowCount, qint64 elapsedMs, qint64 totalRows)
{
    m_storageLabel->setText(QString("%1 条（本批 %2 条 / %3 ms）")
                                .arg(totalRows)
                                .arg(rowCount)
                                .arg(elapsedMs));
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

    m_storageLabel = new QLabel("未启用", this);
    countForm->addRow("已存储：", m_storageLabel);

    auto* dataLayout = new QVBoxLayout;
    dataLayout->addWidget(m_deviceView, 1);
    dataLayout->addLayout(countForm);

    auto* dataGroup = new QGroupBox("实时数据", this);
    dataGroup->setLayout(dataLayout);

    m_chartPanel = new ChartPanel(m_model, this);

    auto* chartGroup = new QGroupBox("实时曲线", this);
    auto* chartLayout = new QVBoxLayout;
    chartLayout->addWidget(m_chartPanel);
    chartGroup->setLayout(chartLayout);

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(connGroup);
    layout->addWidget(dataGroup, 2);
    layout->addWidget(chartGroup, 3);
    layout->addWidget(m_logEdit, 1);
}

void MainWindow::onConnectClicked()
{
    if (m_client->isActive()) {
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

    refreshConnectionUi();
}

void MainWindow::onReconnectScheduled(int delayMs)
{
    Q_UNUSED(delayMs);
    refreshConnectionUi();
}

void MainWindow::refreshConnectionUi()
{
    const bool connected = m_client->isConnected();
    const bool active    = m_client->isActive();

    if (connected)
        m_stateLabel->setText(QStringLiteral("已连接"));
    else if (active) {
        const int remainMs = m_client->pendingReconnectMs();
        if (remainMs > 0)
            m_stateLabel->setText(QString("重连中（%1 秒）").arg((remainMs + 999) / 1000));
        else
            m_stateLabel->setText(QStringLiteral("重连中…"));
    }
    else
        m_stateLabel->setText(QStringLiteral("未连接"));

    if (connected)
        m_connectButton->setText(QStringLiteral("断开"));
    else if (active)
        m_connectButton->setText(QStringLiteral("取消重连"));
    else
        m_connectButton->setText(QStringLiteral("连接"));

    m_hostEdit->setEnabled(!active);
    m_portSpinBox->setEnabled(!active);
}

void MainWindow::onSampleReceived(const Sample&)
{
    ++m_sampleCount;
    m_countLabel->setText(QString::number(m_sampleCount));
}

void MainWindow::onAlarmChanged(int deviceId, Alarm::Level level, const QString& reason)
{
    if (level == Alarm::Level::Normal) {
        appendLog(QString("设备 %1 报警解除").arg(deviceId));
        return;
    }
    appendLog(QString("设备 %1 进入【%2】%3")
                  .arg(deviceId)
                  .arg(Alarm::levelName(level), reason));
}

void MainWindow::appendLog(const QString& text)
{
    const QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logEdit->appendPlainText(QString("[%1] %2").arg(time, text));
}