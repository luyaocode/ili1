#include "clipboardmanager.h"
#include <QApplication>
#include <QSystemTrayIcon>
#include <QDebug>
#include "globaltool.h"
#include "keyblocker.h"

int main(int argc, char *argv[])
{
    if (isAlreadyRunning("PasteFlow"))
    {
        qDebug() << "PasteFlow is already running.";
        return 1;
    }
    QApplication app(argc, argv);

    qRegisterMetaType<KeyWithModifier>("KeyWithModifier");
    qRegisterMetaType<std::vector<KeyWithModifier>>("std::vector<KeyWithModifier>");

    // 检查系统托盘是否可用
    if (!QSystemTrayIcon::isSystemTrayAvailable())
    {
        return 1;
    }
    app.setQuitOnLastWindowClosed(false);  // 关闭最后一个窗口不退出程序

    ClipboardManager manager;   // 剪贴板管理器
    manager.startMonitoring();  // 开始监听剪贴板

    return app.exec();
}
