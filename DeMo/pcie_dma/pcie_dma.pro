# Qt5.9 + C++11 工程配置
QT       += core gui widgets

# 强制启用C++11
CONFIG   += c++11
QMAKE_CXXFLAGS += -std=c++11

# 工程名称
TARGET = PCIeDMADemo
# 生成可执行文件
TEMPLATE = app

DESTDIR =  $$(HOME)/target_dir/DeMo
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

# 源文件列表
SOURCES += main.cpp\
           pciedmawidget.cpp

# 头文件列表
HEADERS  += pciedmawidget.h

# 目标平台：Linux（PCIe DMA仅在Linux下有效）
unix:!macx {
    # 链接必要的系统库
    LIBS += -lpthread -lrt
    # 定义Linux平台宏
    DEFINES += LINUX_PLATFORM
}

# 编译选项：开启警告、调试信息
QMAKE_CXXFLAGS += -Wall -g
QMAKE_LFLAGS += -Wall -g

DISTFILES += \
    README.sh
