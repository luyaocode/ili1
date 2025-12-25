QT       += core gui widgets
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = reminder  # 弹窗程序可执行文件名
TEMPLATE = app

QMAKE_CXXFLAGS += -Wall -Wextra

SOURCES += main.cpp \
    screensaverwindow.cpp \
    cornerpopup.cpp \
    commandhandler.cpp \
    fullscreenpopup.cpp
HEADERS += \
    screensaverwindow.h \
    cornerpopup.h \
    commandhandler.h \
    fullscreenpopup.h

INCLUDEPATH += $$PWD/../commontool

DESTDIR =  $$(HOME)/target_dir/desksrv

LIBS += -L$$DESTDIR -lcommontool
LIBS += -lX11

unix {
    # 同时添加当前目录（$ORIGIN）和 /usr/lib 到 rpath
    QMAKE_LFLAGS += -Wl,-rpath=\'\$$ORIGIN:/usr/lib\'
}

QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

# 安装弹窗程序到/usr/bin
target.path = /usr/bin
INSTALLS += target
