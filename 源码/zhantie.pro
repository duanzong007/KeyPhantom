QT += core gui widgets
CONFIG += c++17 release

!win32:!macx {
    error("KeyPhantom supports Windows and macOS only.")
}

win32 {
    LIBS += -luser32
    RC_ICONS = $$PWD/keyphantom_logo.ico
}

macx {
    LIBS += -framework ApplicationServices -framework Carbon
    ICON = $$PWD/keyphantom_logo.icns
}

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += mainwindow.h
FORMS += mainwindow.ui
RESOURCES += logo.qrc
