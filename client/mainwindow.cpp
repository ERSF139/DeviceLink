#include "mainwindow.h"

#include <QLabel>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("DeviceLink 监控客户端");
    resize(1000, 640);

    auto* titleLabel = new QLabel("监控客户端已启动", this);
    titleLabel->setAlignment(Qt::AlignCenter);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(titleLabel);
}