QT       += core gui widgets x11extras

TARGET = QuickManager
TEMPLATE = app

include($$PWD/../mainconfig.pri)

SOURCES += main.cpp\
           mainwindow.cpp\
           toolmodel.cpp\
           tooldelegate.cpp \
    quickmanager.cpp \
    tool.cpp \
    commonmodel.cpp \
    commondelegate.cpp

HEADERS  += mainwindow.h\
            toolmodel.h\
            tooldelegate.h \
    quickmanager.h \
    tool.h \
    quickitem.h \
    commonmodel.h \
    commondelegate.h


LIBS += -lX11
