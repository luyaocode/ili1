QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = opencv_demo
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    tool.cpp \
    screenrecorder.cpp

HEADERS += \
    mainwindow.h \
    tool.h \
    screenrecorder.h \
    x11struct.h

FORMS += \
    mainwindow.ui

# 配置 OpenCV2 头文件路径（根据实际安装位置调整）
# 系统包管理器安装通常在以下路径
INCLUDEPATH += /usr/include/opencv2 \
               /usr/include

# 配置 OpenCV2 库文件路径
# 系统包管理器安装通常在以下路径
LIBS += -L/usr/lib/x86_64-linux-gnu \
        -L/usr/lib
LIBS += -lX11

# 手动编译安装的库路径（如果适用请取消注释）
# LIBS += -L/usr/local/lib

# 链接 OpenCV2 核心库（根据实际安装的模块调整）
LIBS += -lopencv_core -lopencv_dnn -lopencv_features2d -lopencv_flann -lopencv_imgcodecs \
        -lopencv_highgui -lopencv_imgproc -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_shape \
        -lopencv_stitching -lopencv_superres -lopencv_videoio -lopencv_video -lopencv_videostab

# 启用 C++11 及以上标准
CONFIG += c++11

DESTDIR =  $$(HOME)/target_dir/opencv_demo
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;


# 优化编译选项
QMAKE_CXXFLAGS_RELEASE += -O2 -march=native
