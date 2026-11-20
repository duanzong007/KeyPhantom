#include "appearance.h"
#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("KeyPhantom"));
    QCoreApplication::setApplicationName(QStringLiteral("KeyPhantom"));
    QCoreApplication::setApplicationVersion(QStringLiteral("1.2.0"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("键影 KeyPhantom"));
    applyApplicationAppearance();
#ifdef Q_OS_MACOS
    app.setWindowIcon(QIcon(QStringLiteral(":/keyphantom_logo.icns")));
#else
    app.setWindowIcon(QIcon(QStringLiteral(":/keyphantom_logo.ico")));
#endif
    MainWindow window;
    window.show();
    return app.exec();
}
