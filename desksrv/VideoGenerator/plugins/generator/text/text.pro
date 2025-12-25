QT       += core gui widgets multimedia

include($$PWD/../../../mainconfig.pri)

TARGET = text
TEMPLATE = lib

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    text.cpp

HEADERS +=

DESTDIR =  $$(HOME)/target_dir/desksrv/VideoGenerator/plugins/generator
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;
