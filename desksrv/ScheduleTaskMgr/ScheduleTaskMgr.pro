QT       += core gui widgets x11extras

TARGET = ScheduleTaskMgr
TEMPLATE = app

include($$PWD/../mainconfig.pri)

SOURCES += \
    main.cpp \
    scheduletaskmgr.cpp \
    mainwindow.cpp \
    tool.cpp \
    scheduleitem.cpp \
    schedulemodel.cpp


LIBS += -lX11

HEADERS += \
    scheduletaskmgr.h \
    mainwindow.h \
    tool.h \
    scheduleitem.h \
    schedulemodel.h \
    def.h
