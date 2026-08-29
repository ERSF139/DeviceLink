#include "mainwindow.h"

#include "chartpanel.h"
#include "deviceclient.h"
#include "devicemodel.h"
#include "sampledatabase.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTableView>
#include <QTableWidget>
#include <QTabWidget>
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
    resize(1000, 920);

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
    connect(m_database, &SampleDatabase::queryFinished,
            this, &MainWindow::onQueryFinished);
    connect(m_database, &SampleDatabase::exportFinished,
            this, &MainWindow::onExportFinished);
    connect(m_database, &SampleDatabase::deviceIdsReady,
            this, &MainWindow::onDeviceIdsReady);
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

    auto* realtimePage = new QWidget(this);
    auto* realtimeLayout = new QVBoxLayout(realtimePage);
    realtimeLayout->setContentsMargins(0, 0, 0, 0);
    realtimeLayout->addWidget(dataGroup, 2);
    realtimeLayout->addWidget(chartGroup, 3);

    m_historyDeviceBox = new QComboBox(this);
    m_historyDeviceBox->addItem(QStringLiteral("全部"), 0);

    m_historyFromEdit = new QDateTimeEdit(this);
    m_historyToEdit   = new QDateTimeEdit(this);
    const QDateTime now = QDateTime::currentDateTime();
    m_historyFromEdit->setCalendarPopup(true);
    m_historyToEdit->setCalendarPopup(true);
    m_historyFromEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    m_historyToEdit->setDisplayFormat(QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    m_historyFromEdit->setDateTime(now.addDays(-1));
    m_historyToEdit->setDateTime(now);

    m_historyQueryButton  = new QPushButton(QStringLiteral("查询"), this);
    m_historyExportButton = new QPushButton(QStringLiteral("导出 CSV"), this);

    auto* filterRow = new QHBoxLayout;
    filterRow->addWidget(new QLabel(QStringLiteral("设备："), this));
    filterRow->addWidget(m_historyDeviceBox);
    filterRow->addWidget(new QLabel(QStringLiteral("从"), this));
    filterRow->addWidget(m_historyFromEdit, 1);
    filterRow->addWidget(new QLabel(QStringLiteral("到"), this));
    filterRow->addWidget(m_historyToEdit, 1);
    filterRow->addWidget(m_historyQueryButton);
    filterRow->addWidget(m_historyExportButton);

    m_historyTable = new QTableWidget(0, 5, this);
    m_historyTable->setHorizontalHeaderLabels(
        {QStringLiteral("设备"), QStringLiteral("时间"), QStringLiteral("温度(°C)"),
         QStringLiteral("压力(kPa)"), QStringLiteral("振动(mm/s)")});
    m_historyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_historyTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_historyTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_historyTable->verticalHeader()->setVisible(false);
    m_historyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_historyTable->setAlternatingRowColors(true);

    m_historyStatusLabel = new QLabel(QStringLiteral("尚未查询"), this);

    auto* historyPage = new QWidget(this);
    auto* historyLayout = new QVBoxLayout(historyPage);
    historyLayout->setContentsMargins(0, 0, 0, 0);
    historyLayout->addLayout(filterRow);
    historyLayout->addWidget(m_historyTable, 1);
    historyLayout->addWidget(m_historyStatusLabel);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(realtimePage, QStringLiteral("实时监控"));
    m_tabWidget->addTab(historyPage, QStringLiteral("历史查询"));

    m_logEdit = new QPlainTextEdit(this);
    m_logEdit->setReadOnly(true);
    m_logEdit->setMaximumBlockCount(500);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(connGroup);
    layout->addWidget(m_tabWidget, 5);
    layout->addWidget(m_logEdit, 1);

    connect(m_historyQueryButton, &QPushButton::clicked,
            this, &MainWindow::onHistoryQueryClicked);
    connect(m_historyExportButton, &QPushButton::clicked,
            this, &MainWindow::onHistoryExportClicked);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        if (index == 1) {
            QMetaObject::invokeMethod(m_database, &SampleDatabase::listDeviceIds,
                                      Qt::QueuedConnection);
        }
    });
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

void MainWindow::currentHistoryFilter(int* deviceId, qint64* fromMs, qint64* toMs) const
{
    *deviceId = m_historyDeviceBox->currentData().toInt();
    *fromMs   = m_historyFromEdit->dateTime().toMSecsSinceEpoch();
    *toMs     = m_historyToEdit->dateTime().toMSecsSinceEpoch();
}

void MainWindow::setHistoryBusy(bool busy)
{
    m_historyQueryButton->setEnabled(!busy);
    m_historyExportButton->setEnabled(!busy);
    m_historyDeviceBox->setEnabled(!busy);
    m_historyFromEdit->setEnabled(!busy);
    m_historyToEdit->setEnabled(!busy);
}

void MainWindow::onHistoryQueryClicked()
{
    int deviceId = 0;
    qint64 fromMs = 0;
    qint64 toMs = 0;
    currentHistoryFilter(&deviceId, &fromMs, &toMs);

    setHistoryBusy(true);
    m_historyStatusLabel->setText(QStringLiteral("查询中…"));
    QMetaObject::invokeMethod(m_database, &SampleDatabase::queryHistory,
                              Qt::QueuedConnection, deviceId, fromMs, toMs);
}

void MainWindow::onHistoryExportClicked()
{
    int deviceId = 0;
    qint64 fromMs = 0;
    qint64 toMs = 0;
    currentHistoryFilter(&deviceId, &fromMs, &toMs);

    const QString suggested = QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                                  .filePath(QStringLiteral("devicelink_%1.csv")
                                                .arg(QDateTime::currentDateTime().toString(
                                                    QStringLiteral("yyyyMMdd_HHmmss"))));
    const QString path = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出 CSV"), suggested, QStringLiteral("CSV 文件 (*.csv)"));
    if (path.isEmpty())
        return;

    setHistoryBusy(true);
    m_historyStatusLabel->setText(QStringLiteral("导出中…"));
    QMetaObject::invokeMethod(m_database, &SampleDatabase::exportCsv,
                              Qt::QueuedConnection, path, deviceId, fromMs, toMs);
}

void MainWindow::onQueryFinished(const QList<Sample>& rows, qint64 totalMatched)
{
    setHistoryBusy(false);

    m_historyTable->setSortingEnabled(false);
    m_historyTable->setRowCount(rows.size());
    for (int i = 0; i < rows.size(); ++i) {
        const Sample& sample = rows.at(i);
        const QString time =
            QDateTime::fromMSecsSinceEpoch(sample.timestampMs).toString("yyyy-MM-dd HH:mm:ss");

        auto setCell = [this, i](int column, const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            m_historyTable->setItem(i, column, item);
        };

        setCell(0, QString::number(sample.deviceId));
        setCell(1, time);
        setCell(2, QString::number(sample.temperature, 'f', 2));
        setCell(3, QString::number(sample.pressure, 'f', 2));
        setCell(4, QString::number(sample.vibration, 'f', 3));
    }

    if (totalMatched > rows.size()) {
        m_historyStatusLabel->setText(
            QStringLiteral("显示 %1 / 共 %2 条（表格上限 %3，导出 CSV 可写出全部）")
                .arg(rows.size())
                .arg(totalMatched)
                .arg(SampleDatabase::kMaxQueryRows));
    } else {
        m_historyStatusLabel->setText(QStringLiteral("共 %1 条").arg(totalMatched));
    }
}

void MainWindow::onExportFinished(bool ok, const QString& message)
{
    setHistoryBusy(false);
    m_historyStatusLabel->setText(ok ? message : QStringLiteral("导出失败：%1").arg(message));
}

void MainWindow::onDeviceIdsReady(const QList<int>& ids)
{
    const int current = m_historyDeviceBox->currentData().toInt();
    m_historyDeviceBox->blockSignals(true);
    m_historyDeviceBox->clear();
    m_historyDeviceBox->addItem(QStringLiteral("全部"), 0);
    for (int id : ids)
        m_historyDeviceBox->addItem(QString("设备 %1").arg(id), id);
    const int index = m_historyDeviceBox->findData(current);
    m_historyDeviceBox->setCurrentIndex(index >= 0 ? index : 0);
    m_historyDeviceBox->blockSignals(false);
}