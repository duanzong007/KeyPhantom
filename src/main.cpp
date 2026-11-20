#include "mainwindow.h"
#include <QApplication>
#include <QtPlugin>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifdef Q_OS_MAC
    a.setWindowIcon(QIcon(":/keyphantom_logo.icns"));
#endif

#ifdef Q_OS_WIN
    a.setWindowIcon(QIcon(":/keyphantom_logo.ico"));
#endif

    MainWindow w;
    w.show();
    return a.exec();
}
