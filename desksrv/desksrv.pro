# 桌面服务
TEMPLATE = subdirs
SUBDIRS = monitor reminder ScreenRecorder commontool \
    uinttest \
    PasteFlow \
    qterminal \
    ti \
    EventRecorder \
    StegoTool \
    QuickManager \
    VideoGenerator \
    simkey \
    imagedetector \
    ScheduleTaskMgr \
    Asrv

INCLUDEPATH +=   $$PWD/../unify

# 全局设置 C++11 标准
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

DESTDIR =  $$(HOME)/target_dir/desksrv
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

# 指定依赖关系
monitor.depends = commontool
reminder.depends = commontool
ScreenRecorder.depends = commontool
PasteFlow.depends = commontool
StegoTool.depends = commontool
QuickManager.depends = commontool
ScheduleTaskMgr.depends = commontool
Asrv.depends = commontool
