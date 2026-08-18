#ifndef SIMULATORWINDOW_H
#define SIMULATORWINDOW_H

#include "sample.h"

#include <QWidget>

class Device;
class QLabel;
class QPushButton;

class SimulatorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SimulatorWindow(QWidget* parent = nullptr);

private:
    void buildUi();
    void onSampleGenerated(const Sample& sample);
    void onToggleClicked();

    Device* m_device;

    QLabel* m_nameLabel;
    QLabel* m_statusLabel;
    QLabel* m_temperatureLabel;
    QLabel* m_pressureLabel;
    QLabel* m_vibrationLabel;
    QLabel* m_timestampLabel;

    QPushButton* m_toggleButton;
};

#endif // SIMULATORWINDOW_H