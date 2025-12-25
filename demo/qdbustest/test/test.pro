QT += testlib dbus
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH +=  $$PWD/../../../../codes/EP50Trunk/ComenUltrasound/libs/commonTools/dbus

HEADERS += $$PWD/../../../../codes/EP50Trunk/ComenUltrasound/libs/commonTools/dbus/CDBus.h \

SOURCES +=  $$PWD/../../../../codes/EP50Trunk/ComenUltrasound/libs/commonTools/dbus/CDBus.cpp \
            tst_senddbus.cpp


DESTDIR =  $$(HOME)/target_dir/qdbustest
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;
