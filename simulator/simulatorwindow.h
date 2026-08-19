#ifndef SIMULATORWINDOW_H
#define SIMULATORWINDOW_H

#include "sample.h"

#include <QWidget>

class Device;
class QLabel;
class QPushButton;
class DeviceServer;
class QPlainTextEdit;
class QSpinBox;

class SimulatorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SimulatorWindow(QWidget* parent = nullptr);

private:
    void buildUi();
    void onSampleGenerated(const Sample& sample);
    void onToggleClicked();
    void onListenClicked();
    void onClientCountChanged(int count);
    void appendLog(const QString& text);

    Device* m_device;
    DeviceServer* m_server;

    QLabel* m_nameLabel;
    QLabel* m_statusLabel;
    QLabel* m_temperatureLabel;
    QLabel* m_pressureLabel;
    QLabel* m_vibrationLabel;
    QLabel* m_timestampLabel;

    QPushButton* m_toggleButton;
    QSpinBox*       m_portSpinBox;
    QPushButton*    m_listenButton;
    QLabel*         m_clientCountLabel;
    QPlainTextEdit* m_logEdit;
};

#endif // SIMULATORWINDOW_H