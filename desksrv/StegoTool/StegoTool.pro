QT       += core gui widgets

TARGET = StegoTool
TEMPLATE = app
include($$PWD/../mainconfig.pri)

SOURCES += main.cpp \
           MainWindow.cpp \
           StegoCore.cpp \
    tool.cpp
HEADERS  += MainWindow.h \
            StegoCore.h \
    tool.h

FORMS += \
    mainwindow.ui


