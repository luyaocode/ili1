QT -= core gui
TEMPLATE = lib
TARGET = i_i
CONFIG += c++11

HEADERS += \
    a_a.hpp

SOURCES += \

DESTDIR =  $$(HOME)/target_dir/i_i
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR);
