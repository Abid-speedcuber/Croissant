QT += core gui widgets

CONFIG += c++17
win32: RC_ICONS = icon.ico
RESOURCES += resources.qrc

TARGET   = solve-a-squan
TEMPLATE = app

win32: CONFIG += windows

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    sq1widget.cpp

HEADERS += \
    mainwindow.h \
    sq1widget.h \
    karnotation.h