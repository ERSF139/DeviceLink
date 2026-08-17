#include "simulatorwindow.h"

#include <QLabel>
#include <QVBoxLayout>

SimulatorWindow::SimulatorWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("DeviceLink 设备模拟器");
    resize(480, 360);

    auto* titleLabel = new QLabel("设备模拟器已启动", this);
    titleLabel->setAlignment(Qt::AlignCenter);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
}