#include "mainwindow.h"
#include "sample.h"

#include <QApplication>
#include <QMetaType>

int main(int argc, char* argv[])
{
    qRegisterMetaType<Sample>("Sample");
    qRegisterMetaType<QList<Sample>>("QList<Sample>");
    qRegisterMetaType<QList<int>>("QList<int>");

    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}