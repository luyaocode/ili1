QT -=core gui
TARGET = ti
TEMPLATE = app
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

SOURCES += \
    main.cpp \
    Compressor.cpp \
    tool.cpp

HEADERS += \
    Compressor.h \
    tool.h

DESTDIR =  $$(HOME)/target_dir/desksrv
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

DISTFILES += \
    Readme.sh \
