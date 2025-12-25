TEMPLATE = subdirs
SUBDIRS += \
    StateMachineLib \
    ImageProcessApp

# 依赖顺序：先构建动态库，再构建主程序
ImageProcessApp.depends = StateMachineLib
