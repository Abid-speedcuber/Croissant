QT += core gui widgets

CONFIG += c++17
win32: RC_ICONS = res/icon.ico
RESOURCES += res/resources.qrc

TARGET   = solve-a-squan
TEMPLATE = app

win32: CONFIG += windows

SOURCES += \
    main.cpp \
    sq1-core/output-converter.cpp \
    mainwindow.cpp \
    sq1widget.cpp \
    styles/stylesheet.cpp \
    sq1-core/sq1_logic.cpp

HEADERS += \
    mainwindow.h \
    sq1-core/output-converter.h \
    sq1widget.h \
    sq1-core/karnotation.h \
    styles/stylesheet.h \
    sq1-core/sq1_logic.h