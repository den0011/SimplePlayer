QT       += core gui multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = SimplePlayer
TEMPLATE = app


DEFINES += QT_DEPRECATED_WARNINGS


CONFIG += c++17

SOURCES += \
        clickableslider.cpp \
        main.cpp \
        mainwindow.cpp

HEADERS += \
    clickableslider.h \
    mainwindow.h

FORMS += \
    mainwindow.ui
