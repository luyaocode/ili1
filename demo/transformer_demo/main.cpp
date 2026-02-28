#include <QApplication>
#include "transformer_demo_window.h"

int main(int argc, char *argv[]) {
    // Qt应用程序初始化
    QApplication a(argc, argv);

    // 启用Qt5.9高DPI支持
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    // 创建并显示演示窗口
    TransformerDemoWindow w;
    w.show();

    // 运行应用程序
    return a.exec();
}
