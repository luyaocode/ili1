QT       += core gui network websockets multimedia multimediawidgets widgets

TARGET = webrtc_demo
TEMPLATE = app

# C++11 支持
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11
DEFINES += WEBRTC_POSIX

# WebRTC 头文件路径
INCLUDEPATH += $$PWD/libs/webrtc/include \
               $$PWD/libs/webrtc/include/third_party/abseil-cpp\
               $$PWD/libs/webrtc/include/third_party/boringssl\
               $$PWD/libs/webrtc/include/third_party/jsoncpp\
               $$PWD/libs/webrtc/include/third_party/libyuv\

## 静态库路径
LIBS += -L$$PWD/libs/webrtc/lib \
        -L$$PWD/libs/third_party \
        -lwebrtc \               # 核心 WebRTC 静态库
        -lssl -lcrypto \         # 信令加密
        -lx11 -lxext -lXcomposite \ # 屏幕共享（X11 捕获）
        -lpthread -ldl -lz       # 系统依赖

DESTDIR =  $$(HOME)/target_dir/webrtc_demo
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

# 源文件
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    webrtcwrapper.cpp \
    signalingclient.cpp

HEADERS += \
    webrtcwrapper.h \
    mainwindow.h \
    signalingclient.h

FORMS += \
    mainwindow.ui
