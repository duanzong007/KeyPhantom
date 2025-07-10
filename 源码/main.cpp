#include "mainwindow.h"
#include <QApplication>
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QICOPlugin)


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon("C:/zhantie/keyphantom_logo.ico"));
    MainWindow w;
    w.show();
    return a.exec();
}
