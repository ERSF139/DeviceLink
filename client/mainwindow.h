#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "alarm.h"
#include "sample.h"

#include <QWidget>

class ChartPanel;
class DeviceClient;
class QComboBox;
class QDateTimeEdit;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class DeviceModel;
class QTableView;
class QTableWidget;
class QTabWidget;
class QThread;
class SampleDatabase;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

public slots:
    void onOpened(qint64 totalRows);
    void onFlushed(int rowCount, qint64 elapsedMs, qint64 totalRows);
    void onQueryFinished(const QList<Sample>& rows, qint64 totalMatched);
    void onExportFinished(bool ok, const QString& message);
    void onDeviceIdsReady(const QList<int>& ids);

private:
    void buildUi();
    void onConnectClicked();
    void onConnectedChanged(bool connected);
    void onReconnectScheduled(int delayMs);
    void onSampleReceived(const Sample& sample);
    void onAlarmChanged(int deviceId, Alarm::Level level, const QString& reason);
    void appendLog(const QString& text);
    void refreshConnectionUi();
    void onHistoryQueryClicked();
    void onHistoryExportClicked();
    void currentHistoryFilter(int* deviceId, qint64* fromMs, qint64* toMs) const;
    void setHistoryBusy(bool busy);

    ChartPanel* m_chartPanel;
    DeviceClient* m_client;
    DeviceModel* m_model;
    QTableView* m_deviceView;

    QLineEdit*   m_hostEdit;
    QSpinBox*    m_portSpinBox;
    QPushButton* m_connectButton;
    QLabel*      m_stateLabel;
    QLabel* m_countLabel;

    QPlainTextEdit* m_logEdit;

    QThread*        m_dbThread;
    SampleDatabase* m_database;
    QLabel*         m_storageLabel;

    QTabWidget*     m_tabWidget;
    QComboBox*      m_historyDeviceBox;
    QDateTimeEdit*  m_historyFromEdit;
    QDateTimeEdit*  m_historyToEdit;
    QPushButton*    m_historyQueryButton;
    QPushButton*    m_historyExportButton;
    QTableWidget*   m_historyTable;
    QLabel*         m_historyStatusLabel;

    int m_sampleCount = 0;
};

#endif // MAINWINDOW_H