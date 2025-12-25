#ifndef COMMANDHANDLER_H
#define COMMANDHANDLER_H

#include <QStringList>
#include <QApplication>

//  forward declarations（避免过多头文件包含）
class ScreenSaverWindow;
class CornerPopup;
class FullscreenPopup;

class CommandHandler
{
public:
    // 构造函数
    CommandHandler(QApplication &app);  // 接收QApplication引用

    // 处理参数并执行对应逻辑
    bool processArguments(const QStringList &args);
    // 执行对应命令的窗口逻辑（需要在processArguments之后调用）
    int execute();

private:
    // 辅助方法
    void showHelp();
    void showVersion();
    void showUsage();

private:
    QApplication &m_app;      // 引用应用实例，用于执行事件循环
    bool          m_exit;     // 是否退出程序
    QString       m_command;  // 存储命令（如"screensaver"、"cornerpopup"）
    QString       m_info;     // 存储cornerpopup的消息参数
};

#endif  // COMMANDHANDLER_H
