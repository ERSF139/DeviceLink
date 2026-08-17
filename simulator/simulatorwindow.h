#ifndef SIMULATORWINDOW_H
#define SIMULATORWINDOW_H

#include <QWidget>

class SimulatorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SimulatorWindow(QWidget* parent = nullptr);
};

#endif // SIMULATORWINDOW_H