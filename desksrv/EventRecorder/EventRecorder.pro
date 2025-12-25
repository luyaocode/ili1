QT       += core gui widgets x11extras

TARGET = EventRecorder
TEMPLATE = app

CONFIG += c++11 console

SOURCES += main.cpp \
           x11_event_recorder.cpp \
           x11_event_player.cpp \
           event_serializer.cpp \
           application_controller.cpp

HEADERS  += \
            x11_event_recorder.h \
            x11_event_player.h \
            recorded_event.h \
            event_serializer.h \
            application_controller.h \
    x11struct.h

# 链接 X11 和 XTest 库
LIBS += -lX11 -lXtst

DESTDIR =  $$(HOME)/target_dir/desksrv
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;
