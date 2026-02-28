# 项目名称
QT += core gui widgets charts

# C++版本约束
CONFIG += c++11

# 禁止Qt弃用警告（适配Qt5.9）
DEFINES += QT_DEPRECATED_WARNINGS

# 目标程序名称
TARGET = TransformerDemo

DESTDIR =  $$(HOME)/target_dir/demo/TransformerDemo
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

# 源文件列表
SOURCES += main.cpp \
           transformer_core.cpp \
           transformer_demo_window.cpp
# 头文件列表
HEADERS += transformer_core.h \
           transformer_demo_window.h

