#include <QApplication>
#include "mmapfile.h"
#include "mainwindow.h"
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 启动主窗口
    MainWindow w;
    w.show();

    return a.exec();
}
