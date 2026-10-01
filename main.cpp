#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName("filesend");   // settings -> ~/.config/filesend/ on Linux
    QApplication::setApplicationName("filesend");

    MainWindow w;
    w.show();
    return app.exec();
}
