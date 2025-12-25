#include "qterminalwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setApplicationName("QTerminal");  // 应用名称同步更新

    QTerminalWidget w;
    w.resize(800, 600);  // 设置初始窗口大小
    w.show();

    return a.exec();
}
