QT       += core gui widgets          # 需要的 Qt 模块

# -------- 构建选项 --------
CONFIG   += static release c++17      # 静态 + Release + C++17
DEFINES  += QT_STATIC                 # 静态宏

LIBS     += -luser32                  # Windows 系统库

# -------- 嵌入 ICO 图标 --------
win32 {
    RC_ICONS = C:/zhantie/keyphantom_logo.ico
}

# -------- 将必须插件编进 EXE --------
QTPLUGIN += qwindows qico             # 平台插件 + ICO 插件

# -------- 源码 --------
SOURCES  += \
    main.cpp \
    mainwindow.cpp

HEADERS  += \
    mainwindow.h

FORMS    += \
    mainwindow.ui
