#include "mainwindow.h"
#include "sample.h"

#include <QApplication>
#include <QMetaType>

int main(int argc, char* argv[])
{
    qRegisterMetaType<Sample>("Sample");

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}