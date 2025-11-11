QT       += core gui multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ScreenRecorder
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    screenrecorder.cpp

HEADERS += \
    mainwindow.h \
    screenrecorder.h

FORMS += \
    mainwindow.ui

# 确保支持中文显示
DEFINES += QT_DEPRECATED_WARNINGS

# Windows平台特定配置
win32 {
    # 链接Windows多媒体库
    LIBS += -lwinmm
}

# 输出目录设置
DESTDIR = ./bin
OBJECTS_DIR = ./build
MOC_DIR = ./build
RCC_DIR = ./build
