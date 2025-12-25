QT       += core scxml  # 仅依赖 core 和 scxml（无需 widgets）
TEMPLATE  = lib          # 类型为动态库

# 导出宏定义（与 statemachine_global.h 对应）
DEFINES += STATEMACHINE_LIBRARY

# 库名称（生成的库文件名为：libStateMachineLib.so/.dll）
TARGET = StateMachineLib

DESTDIR =  $$(HOME)/target_dir/demo
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

STATECHARTS += \
    ImageProcess.scxml


# 库的源文件和头文件
SOURCES += \
    ImageProcessState.cpp \
    ImageProcessDataModel.cpp

HEADERS  += \
    ImageProcessState.h \
    statemachine_global.h \
    ImageProcessDataModel.h

