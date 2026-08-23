#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "alarm.h"
#include "sample.h"

#include <QWidget>

class ChartPanel;
class DeviceClient;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class DeviceModel;
class QTableView;

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void buildUi();
    void onConnectClicked();
    void onConnectedChanged(bool connected);
    void onSampleReceived(const Sample& sample);
    void onAlarmChanged(int deviceId, Alarm::Level level, const QString& reason);
    void appendLog(const QString& text);

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

    int m_sampleCount = 0;
};

#endif // MAINWINDOW_H