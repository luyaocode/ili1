#include "commandhandler.h"

#include <QDebug>
#include <QTextStream>
#include <QCoreApplication>

#include "screensaverwindow.h"
#include "cornerpopup.h"
#include "fullscreenpopup.h"

// 构造函数：初始化应用引用和成员变量
CommandHandler::CommandHandler(QApplication &app): m_app(app), m_exit(true), m_command(""), m_info("")
{
}

bool CommandHandler::processArguments(const QStringList &args)
{
    // 解析参数
    if (args.contains("--help") || args.contains("-h"))
    {
        showHelp();
        m_exit = true;
        return true;
    }

    if (args.contains("--version") || args.contains("-v"))
    {
        showVersion();
        m_exit = true;
        return true;
    }

    // 处理"screensaver"命令
    if (args.contains("screensaver"))
    {
        m_exit    = false;
        m_command = "screensaver";
        return true;
    }

    // 处理"cornerpopup"命令（带消息参数）
    int cornerIndex = args.indexOf("cornerpopup");
    if (cornerIndex != -1 && cornerIndex + 1 < args.size())
    {
        m_exit    = false;
        m_command = "cornerpopup";
        m_info    = args[cornerIndex + 1];  // 获取消息参数
        return true;
    }

    // 无参数或未知参数：默认显示全屏弹窗
    if (args.size() == 1)
    {  // 仅程序名，无其他参数
        m_exit    = false;
        m_command = "fullscreen";
        return true;
    }

    // 未知参数：显示用法并退出
    showUsage();
    m_exit = true;
    return false;
}

// 执行对应命令的窗口逻辑（需要在processArguments之后调用）
int CommandHandler::execute()
{
    if (m_exit)
    {
        return 0;  // 退出程序
    }

    // 根据解析的命令执行不同窗口
    if (m_command == "screensaver")
    {
        ScreenSaverWindow window("");
        window.show();
        return m_app.exec();  // 启动事件循环
    }
    else if (m_command == "cornerpopup")
    {
        CornerPopup *d = new CornerPopup(m_info);
        d->show();
        return m_app.exec();
    }
    else if (m_command == "fullscreen")
    {
        FullscreenPopup popup;
        popup.showFullScreen();
        return m_app.exec();
    }

    return 0;
}

// 以下是原有辅助方法的实现
void CommandHandler::showHelp()
{
    qInfo() << "reminder - 提醒弹窗";
    qInfo() << "用法:";
    qInfo() << "  reminder [选项] [参数]";
    qInfo() << "选项:";
    qInfo() << "  -h, --help               显示帮助信息";
    qInfo() << "  -v, --version            显示版本信息";
    qInfo() << "  screensaver              激活屏保";
    qInfo() << "  cornerpopup [提示消息]    普通文本弹窗";
}

void CommandHandler::showVersion()
{
    qInfo() << QCoreApplication::applicationName() << " 版本: " << QCoreApplication::applicationVersion();
}

void CommandHandler::showUsage()
{
    QTextStream out(stdout);
    out << "Usage: reminder [options]" << endl << endl;
    out << "Options:" << endl;
    out << "  -h, --help           Show this help message and exit" << endl;
    out << "  -v, --version        Show version information and exit" << endl;
    out << "  screensaver          Activate screensaver" << endl;
    out << "  cornerpopup [info]   Show notification" << endl;
}
