QT       += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qterminal  # 直观的项目名称
TEMPLATE = app

SOURCES += \
    main.cpp \
    qterminalwidget.cpp \

HEADERS += \
    qterminalwidget.h \

DESTDIR =  $$(HOME)/target_dir/desksrv
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

# 启用 C++11
QMAKE_CXXFLAGS += -std=c++11

LIBS += -lutil  # 终端相关辅助库

unix {
    QMAKE_LFLAGS += -Wl,-rpath=\'\$$ORIGIN:/usr/lib\'
}
