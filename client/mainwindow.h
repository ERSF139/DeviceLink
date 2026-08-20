#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "sample.h"

#include <QWidget>

class DeviceClient;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;

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
    void appendLog(const QString& text);

    DeviceClient* m_client;

    QLineEdit*   m_hostEdit;
    QSpinBox*    m_portSpinBox;
    QPushButton* m_connectButton;
    QLabel*      m_stateLabel;

    QLabel* m_deviceIdLabel;
    QLabel* m_temperatureLabel;
    QLabel* m_pressureLabel;
    QLabel* m_vibrationLabel;
    QLabel* m_timestampLabel;
    QLabel* m_countLabel;

    QPlainTextEdit* m_logEdit;

    int m_sampleCount = 0;
};

#endif // MAINWINDOW_H