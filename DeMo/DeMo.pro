TEMPLATE = subdirs
SUBDIRS = pcie_dma \
    mmap

# 全局设置 C++11 标准
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

# 指定依赖关系
#monitor.depends = commontool
