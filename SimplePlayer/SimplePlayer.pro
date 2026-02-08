QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SimplePlayer
TEMPLATE = app


DEFINES += QT_DEPRECATED_WARNINGS


CONFIG += c++17

SOURCES += \
        main.cpp \
        VideoPlayer.cpp

HEADERS += \
    VideoPlayer.h

FORMS += \
    VideoPlayer.ui
