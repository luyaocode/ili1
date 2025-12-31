#-------------------------------------------------
#
# Project created by QtCreator 2025-10-22T17:51:27
#
#-------------------------------------------------

QT       += core widgets

TARGET = commontool
TEMPLATE = lib

DEFINES += COMMONTOOL_LIBRARY

CONFIG(debug, debug|release) {
    # Debug 模式专属宏
    DEFINES += DEBUG
    message(当前编译模式: debug)
} else {
    # Release 模式专属宏
    DEFINES += RELEASE
    message(当前编译模式: release)
}

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS


QMAKE_CXXFLAGS += -frtti  # 启用 C++ RTTI

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        commontool.cpp \
    multischeduledtaskmgr.cpp \
    keyblocker.cpp \
    globaltool.cpp \
    mousesimulator.cpp \
    virtualmousewidget.cpp \
    VersionManager.cpp \
    UpdateDialog.cpp \
    screenshooter.cpp

HEADERS += \
        commontool.h \
        commontool_global.h \   
    multischeduledtaskmgr.h \
    keyblocker.h \
    globaltool.h \
    widgetfilter.hpp \
    x11struct.h \
    mousesimulator.h \
    virtualmousewidget.h \
    VersionManager.h \
    UpdateDialog.h \
    drmstruct.h \
    screenshooter.h \
    globaldef.h

LIBS += -lX11 -lXtst -ldrm -lpthread

DESTDIR =  $$(HOME)/target_dir/desksrv
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

unix {
    target.path = /usr/lib
    INSTALLS += target
}
