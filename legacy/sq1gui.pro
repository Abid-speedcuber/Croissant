QT += core gui widgets

CONFIG += c++17
DEFINES += SQ1OPT_LIBRARY
INCLUDEPATH += sq1-core
win32: RC_ICONS = res/icon.ico
RESOURCES += res/resources.qrc

TARGET   = croissant
TEMPLATE = app

win32: CONFIG += windows

SOURCES += \
    main.cpp \
    sq1-core/output-converter.cpp \
    mainwindow.cpp \
    ../main/src-tauri/native/sq1opt.cpp \
    sq1widget.cpp \
    styles/stylesheet.cpp \
    sq1-core/sq1-logic.cpp

HEADERS += \
    mainwindow.h \
    sq1-core/output-converter.h \
    sq1widget.h \
    sq1-core/karnotation.h \
    sq1-core/sq1opt-runner.h \
    styles/stylesheet.h \
    sq1-core/sq1-logic.h
