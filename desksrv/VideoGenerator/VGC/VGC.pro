QT       += core gui widgets multimedia multimediawidgets

TARGET = VideoGenerator
TEMPLATE = app

include($$PWD/../mainconfig.pri)

SOURCES += main.cpp \
           MainWindow.cpp

HEADERS  += MainWindow.h

LIBS += -ldl
