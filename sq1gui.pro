QT += core gui widgets

CONFIG += c++17

TARGET   = sq1gui
TEMPLATE = app

# On Windows: hide the console window completely
win32: CONFIG += windows

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    sq1widget.cpp

HEADERS += \
    mainwindow.h \
    sq1widget.h