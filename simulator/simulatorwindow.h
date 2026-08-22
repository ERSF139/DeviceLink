#ifndef SIMULATORWINDOW_H
#define SIMULATORWINDOW_H

#include "sample.h"

#include <QList>
#include <QWidget>

class Device;
class QLabel;
class QPushButton;
class DeviceServer;
class QPlainTextEdit;
class QSpinBox;
class QTableWidget;

class SimulatorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SimulatorWindow(QWidget* parent = nullptr);

private:
    void buildUi();
    void createDevices();

    void onSampleGenerated(const Sample& sample);
    void onToggleAllClicked();
    void onToggleSelectedClicked();
    void onListenClicked();
    void onClientCountChanged(int count);
    void appendLog(const QString& text);

    bool anyDeviceRunning() const;
    void updateControls();
    void refreshStatusCell(int row);

    QList<Device*> m_devices;
    DeviceServer* m_server;

    QTableWidget* m_deviceTable;
    QPushButton*  m_toggleAllButton;
    QPushButton*  m_toggleSelectedButton;

    QSpinBox*       m_portSpinBox;
    QPushButton*    m_listenButton;
    QLabel*         m_clientCountLabel;
    QPlainTextEdit* m_logEdit;
};

#endif // SIMULATORWINDOW_H