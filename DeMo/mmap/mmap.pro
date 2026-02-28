QT += core gui widgets

CONFIG += c++11
TARGET = mmap
TEMPLATE = app

DESTDIR =  $$(HOME)/target_dir/DeMo
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

SOURCES += main.cpp \
           mmapfile.cpp \
    mainwindow.cpp

HEADERS += mmapfile.h \
    mainwindow.h
