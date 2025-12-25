QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
# 启用 C++11 及以上标准
CONFIG += c++11

TARGET = ScreenRecorder
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    tool.cpp \
    screenrecorder.cpp \
    commandhandler.cpp

HEADERS += \
    mainwindow.h \
    tool.h \
    screenrecorder.h \
    x11struct.h \
    commandhandler.h

FORMS += \
    mainwindow.ui

DESTDIR =  $$(HOME)/target_dir/desksrv
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

# 配置 OpenCV2 头文件路径（根据实际安装位置调整）
# 系统包管理器安装通常在以下路径
INCLUDEPATH += /usr/include/opencv2 \
               /usr/include
# 通用工具库
INCLUDEPATH += $$PWD/../commontool

# 配置 OpenCV2 库文件路径

LIBS += -L$$DESTDIR -lcommontool
LIBS += -lX11

# 系统包管理器安装通常在以下路径
LIBS += -L/usr/lib/x86_64-linux-gnu \
        -L/usr/lib

# 链接 OpenCV2 核心库（根据实际安装的模块调整）
LIBS += -lopencv_core -lopencv_dnn -lopencv_features2d -lopencv_flann -lopencv_imgcodecs \
        -lopencv_highgui -lopencv_imgproc -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_shape \
        -lopencv_stitching -lopencv_superres -lopencv_videoio -lopencv_video -lopencv_videostab


# 优化编译选项，使用通用指令集
QMAKE_CXXFLAGS_RELEASE += -O2 -march=x86-64 -mtune=generic

unix {
    # 同时添加当前目录（$ORIGIN）和 /usr/lib 到 rpath
    QMAKE_LFLAGS += -Wl,-rpath=\'\$$ORIGIN:/usr/lib\'
}

QMAKE_POST_LINK = $$QMAKE_POST_LINK



