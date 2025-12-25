QT += core gui widgets

CONFIG += c++11

TARGET = crossbox

TEMPLATE = app

SOURCES += main.cpp \
    ballwindow.cpp \
    ball.cpp

DESTDIR =  $$(HOME)/target_dir/crossbox
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

HEADERS += \
    ballwindow.h \
    ball.h \
    inc.h
