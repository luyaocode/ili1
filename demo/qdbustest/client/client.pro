QT       += core dbus
QT       -= gui  # 不依赖 GUI

CONFIG   += c++11
TARGET    = dbus_client  # 服务端可执行文件名
TEMPLATE  = app

INCLUDEPATH +=  $$PWD/../../../../codes/EP50Trunk/ComenUltrasound/libs/commonTools/dbus

HEADERS     +=  $$PWD/../../../../codes/EP50Trunk/ComenUltrasound/libs/commonTools/dbus/CDBus.h \
                client.h \

SOURCES     +=  $$PWD/../../../../codes/EP50Trunk/ComenUltrasound/libs/commonTools/dbus/CDBus.cpp \
                main.cpp \
                client.cpp \


DESTDIR =  $$(HOME)/target_dir/qdbustest
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;
