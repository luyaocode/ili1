QT       += core gui widgets multimedia

include($$PWD/../../../mainconfig.pri)

TARGET = grayscale
TEMPLATE = lib

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
    grayscale.cpp

HEADERS +=

DESTDIR =  $$(HOME)/target_dir/desksrv/VideoGenerator/plugins/filter
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;
