QT       += core gui widgets dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = monitor
TEMPLATE = app

# 编译选项
QMAKE_CXXFLAGS += -Wall -Wextra

contains(CONFIG,qml_debug){
    DEFINES+=DEBUG
}
#message("current_config: $$CONFIG")
#message("current_define: $$DEFINES")

# 源文件
SOURCES += \
    main.cpp \
    monitor.cpp \
    inputwatcher.cpp \
    datamanager.cpp \
    commandhandler.cpp \
    guimanager.cpp \
    configparser.cpp \
    app.cpp \
    eyesprotector.cpp \
    extprocess.cpp

# 头文件
HEADERS += \
    monitor.h \
    inputwatcher.h \
    datamanager.h \
    commandhandler.h \
    guimanager.h \
    x11struct.h \
    configparser.h \
    globaldefine.h \
    app.h \
    eyesprotector.h \
    extprocess.h


DESTDIR =  $$(HOME)/target_dir/desksrv
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

INCLUDEPATH += $$PWD/../commontool
LIBS += -L$$DESTDIR -lcommontool

LIBS += -lX11

unix {
    QMAKE_LFLAGS += -Wl,-rpath=\'\$$ORIGIN:/usr/lib\'
}

# 安装配置
target.path = /usr/bin
INSTALLS += target

# 系统服务文件
service.files = monitor.service
service.path = /etc/systemd/system
INSTALLS += service

# 配置文件
config.files = monitor.conf
config.path = /etc
INSTALLS += config

DISTFILES += \
    monitor.service \
    monitor.conf \
    README.sh \
    install.sh \
    package.sh
# 编译后拷贝文件到输出目录
FILES_TO_COPY = \
    $$PWD/monitor.service \
    $$PWD/monitor.conf \
    $$PWD/install.sh \
    $$PWD/package.sh \
    $$PWD/README.sh
for(f, FILES_TO_COPY) {
    QMAKE_POST_LINK += $$quote(cp -f $$shell_path($$f) $$shell_path($$DESTDIR)$$escape_expand(\\n\\t))
}

!contains(DEFINES, DEBUG) {
    QMAKE_POST_LINK += strip $$DESTDIR/$$TARGET
}

QMAKE_POST_LINK = $$QMAKE_POST_LINK
