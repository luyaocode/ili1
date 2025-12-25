QT       += core

TARGET = calc_v2
TEMPLATE = lib

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
#    calc_v1.cpp \
    calc_v2.cpp

HEADERS += \
    calc_interface.h

DESTDIR =  $$(HOME)/target_dir/demo
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;
