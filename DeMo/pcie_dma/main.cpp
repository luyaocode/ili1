#include <QApplication>
#include "pciedmawidget.h"
#include <QTextCodec>

/**
 * @brief 主函数：程序入口
 * @param argc 命令行参数个数
 * @param argv 命令行参数数组
 * @return 程序退出码
 */
int main(int argc, char *argv[])
{
    // Qt应用程序初始化
    QApplication a(argc, argv);

    // 兼容中文（Qt5.9 Linux下）
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    // 创建并显示主窗口
    PCIeDMAWidget w;
    w.show();

    // 运行应用程序事件循环
    return a.exec();
}
