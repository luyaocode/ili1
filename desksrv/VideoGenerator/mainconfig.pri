INCLUDEPATH += $$PWD/../../include

# 全局 C++11 配置
CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++11

INCLUDEPATH += $$PWD/include

DESTDIR =  $$(HOME)/target_dir/desksrv/VideoGenerator
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;

LIBS += -L$$DESTDIR/../ -lcommontool
unix {
    # 同时添加当前目录（$ORIGIN）和 /usr/lib 到 rpath
    QMAKE_LFLAGS += -Wl,-rpath=\'\$$ORIGIN:/usr/lib\'
}

QMAKE_POST_LINK = $$QMAKE_POST_LINK


# 其他全局配置（如 DEFINES、LIBS 等）
# DEFINES += QT_NO_DEBUG_OUTPUT
