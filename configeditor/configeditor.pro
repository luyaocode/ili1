QT       += core gui widgets xml concurrent
QT       -= console
TARGET = ConfigEditor
TEMPLATE = app
CONFIG += c++11
SOURCES += main.cpp \
           MainWindow.cpp \
           FileExplorer.cpp \
           XmlViewer.cpp \
           TextViewer.cpp \
    editablelabel.cpp \
    customxmltreeheader.cpp \
    ConfigManager.cpp \
    Tool.cpp \
    AttrViewer.cpp \
    richtextdelegate.cpp \
    buttontoolbar.cpp
HEADERS  += MainWindow.h \
            FileExplorer.h \
            XmlViewer.h \
            TextViewer.h \
    editablelabel.h \
    customxmltreeheader.h \
    ConfigManager.h \
    Tool.h \
    AttrViewer.h \
    richtextdelegate.h \
    buttontoolbar.h


DESTDIR =  $$(HOME)/target_dir/configeditor
# 不存在目标目录就先创建
QMAKE_POST_LINK += mkdir -p $$shell_quote($$DESTDIR) ;
