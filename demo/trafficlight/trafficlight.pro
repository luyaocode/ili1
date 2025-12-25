QT += core
QT -= gui

CONFIG += c++11

TARGET = trafficlight
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

DESTDIR =  $$(HOME)/target_dir/trafficlight
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;
