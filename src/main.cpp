#include "appearance.h"
#include "mainwindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("KeyPhantom"));
    QCoreApplication::setApplicationName(QStringLiteral("KeyPhantom"));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(KEYPHANTOM_VERSION));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("键影"));
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
