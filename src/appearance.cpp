#include "appearance.h"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>

void applyApplicationAppearance()
{
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QPalette palette;
    palette.setColor(QPalette::Window, QColor("#F4F5F9"));
    palette.setColor(QPalette::WindowText, QColor("#263247"));
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::AlternateBase, QColor("#F7F8FC"));
    palette.setColor(QPalette::Text, QColor("#263247"));
    palette.setColor(QPalette::PlaceholderText, QColor("#9BA4B6"));
    palette.setColor(QPalette::Button, Qt::white);
    palette.setColor(QPalette::ButtonText, QColor("#536077"));
    palette.setColor(QPalette::Highlight, QColor("#E1DCF8"));
    palette.setColor(QPalette::HighlightedText, QColor("#30254F"));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor("#ACB3C2"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor("#ACB3C2"));
    QApplication::setPalette(palette);
    QFile stylesheet(QStringLiteral(":/style.qss"));
    if (stylesheet.open(QIODevice::ReadOnly))
        qApp->setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
}
