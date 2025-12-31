greaterThan(QT_MAJOR_VERSION, 4): QT += core network websockets gui widgets

include($$PWD/../mainconfig.pri)


CONFIG(debug, debug|release) {
    # Debug 模式专属宏
    DEFINES += DEBUG
    message(当前编译模式: debug)
} else {
    # Release 模式专属宏
    DEFINES += RELEASE
    message(当前编译模式: release)
}

DESTDIR =  $$(HOME)/target_dir/desksrv/Asrv
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

TARGET = Asrv
TEMPLATE = app

SOURCES += main.cpp \
    tool.cpp \
    TcpServer.cpp \
    WebSocketServer.cpp \
    Server.cpp \
    widget/NotifyPopup.cpp \
    ScreenServer.cpp \
    x11tool.cpp

HEADERS += \
    tool.h \
    def.h \
    unicode.h \
    TcpServer.h \
    WebSocketServer.h \
    Server.h\
    widget/NotifyPopup.h \
    ScreenServer.h \
    x11tool.h \
    ClientInfo.h

LIBS += -lutil

DISTFILES += \
    www/css/style.css\
    www/403.html \
    www/404.html \
    www/dir_list.html \
    www/js/file_browser.js \
    www/500.html \
    www/screen.html \
    www/js/test_ws.js \
    www/test_ws.html \
    www/bash.html \
    www/js/bash.js \
    www/control.html \
    www/css/control.css \
    www/js/control.js \
    www/xterm.html \
    www/js/xterm.min.js \
    www/css/xterm.css \
    www/js/xterm-addon-attach.js \
    www/js/xterm-addon-fit.js \
    www/js/xterm-addon-fit.min.js \
    www/js/xterm.js \
    www/js/term.min.js \
    www/js/xtermpage.js \
    www/screen_ctrl.html \
    www/js/screen_ctrl.js \
    www/css/screen_ctrl.css \
    version.json \
    www/js/termpage.js \
    run.sh

unix: {
    QMAKE_POST_LINK += rm -rf $$shell_path($$DESTDIR/www) ; \
                       cp -rf $$shell_path($$PWD/www) $$shell_path($$DESTDIR) ; \
                       cp -d $$shell_path($$DESTDIR/../libcommontool.so*) $$shell_path($$DESTDIR) ; \
                       cp -rf $$shell_path($$PWD/version.json $$PWD/run.sh) $$shell_path($$DESTDIR) ;
}

