#include <QCoreApplication>
#include <QApplication>
#include <QDebug>
#include <QProcess>
#include <QSettings>
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include <QTextStream>
#include "globaldefine.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>  // 提供 fopen()、fprintf()、fclose() 等文件操作函数
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <iostream>
#include "monitor.h"
#include "commandhandler.h"
#include "guimanager.h"
#include "configparser.h"
#include "app.h"
#include "globaltool.h"

bool g_showLog = true;
// 检查是否以root权限运行
bool checkRoot()
{
    return getuid() == 0;
}

// 检查是否已有实例在运行
//bool isAlreadyRunning()
//{
//    // 简单的进程检查实现
//    QProcess process;
//    process.start("pgrep", QStringList() << "monitor");
//    process.waitForFinished();

//    QString     output = process.readAllStandardOutput();
//    QStringList pids   = output.split('\n', QString::SkipEmptyParts);

//    // 如果找到多个PID，说明已有实例运行
//    return pids.size() > 1;
//}

// 全局日志处理函数：自动添加线程ID、时间、日志级别
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    UNUSED(context)
    if(!g_showLog)
    {
        return;
    }
    // 1. 获取当前时间（格式：yyyy-MM-dd hh:mm:ss.zzz）
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 2. 获取线程名称ID
    QString       tname;
    unsigned long thread_id = reinterpret_cast<unsigned long>(pthread_self());
    tname                   = QString::number(thread_id);

    // 3. 日志级别字符串
    QString level;
    QString logText;
    switch (type)
    {
        case QtDebugMsg:
            level = "DEBUG";
            break;
        case QtInfoMsg:
        {
            level = "INFO";
            logText = msg;
            break;
        }
        case QtWarningMsg:
            level = "WARN";
            break;
        case QtCriticalMsg:
            level = "ERROR";
            break;
        case QtFatalMsg:
            level = "FATAL";
            break;
    }
    if(logText.isEmpty())
    {
        // 4. 格式化日志并输出到控制台
        logText= QString("[%1] %2 [%3] %4")
                              .arg(tname)  // 线程ID
                              .arg(time)   // 时间
                              .arg(level)  // 级别
                              .arg(msg);   // 日志内容
    }


    QTextStream(stdout) << logText << endl;

    // 致命错误时终止程序（保持Qt默认行为）
    if (type == QtFatalMsg)
    {
        abort();
    }
}

// 处理子进程退出信号，防止僵尸进程
void handle_sigchld(int sig)
{
    (void)sig;
    pid_t pid;
    // 非阻塞方式回收所有已退出的子进程
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0)
        ;
}

// 关闭所有可能打开的文件描述符.可能会把x11用到的fd关闭
void close_all_fds()
{
    int max_fd = sysconf(_SC_OPEN_MAX);
    for (int fd = 3; fd < max_fd; fd++)
    {
        close(fd);
    }
}

// 守护进程初始化函数
bool daemonize()
{
    // 注册信号处理函数，防止僵尸进程
    struct sigaction sa;
    sa.sa_handler = handle_sigchld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;  // 重启被中断的系统调用
    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
        return false;
    }

    // 第一次 fork：脱离父进程
    pid_t pid = fork();
    if (pid < 0)
    {
        return false;  // fork 失败
    }
    else if (pid > 0)
    {
        exit(0);  // 父进程退出
    }

    // 创建新会话，脱离终端控制
    if (setsid() == -1)
    {
        return false;
    }

    // 第二次 fork：确保进程不是会话组长，无法打开终端
    pid = fork();
    if (pid < 0)
    {
        return false;  // fork 失败
    }
    else if (pid > 0)
    {
        exit(0);  // 第一个子进程退出
    }

    // 切换工作目录到根目录，避免占用可卸载的文件系统
    if (chdir("/") == -1)
    {
        return false;
    }

    // 重置文件权限掩码
    umask(0);

    // 关闭所有不必要的文件描述符,可能关闭x11用到的fd
//    close_all_fds();

    // 重定向标准输入输出到 /dev/null
    int dev_null = open("/dev/null", O_RDWR);
    if (dev_null == -1)
    {
        return false;
    }
    dup2(dev_null, STDIN_FILENO);  // 重定向 stdin
    close(dev_null);

    return true;
}

int main(int argc, char *argv[])
{
    QStringList rawArgs;
    bool        needGui = false;
    // 处理命令行参数
    CommandHandler cmdHandler;
    for (int i = 0; i < argc; ++i)
    {
        rawArgs << argv[i];
    }
    if (rawArgs.contains("--query") || rawArgs.contains("-q"))
    {
        needGui = rawArgs.contains("--gui") || rawArgs.contains("-g");
    }

    QCoreApplication *app = nullptr;
    if (needGui)
    {
        app = new QApplication(argc, argv);
        g_showLog = false;
    }
    else
    {
        app = new QCoreApplication(argc, argv);
    }
    app->setApplicationName("monitor");
    app->setApplicationVersion("1.0");
    int result = 0;
    // 注册日志监听
    qInstallMessageHandler(customMessageHandler);
    if (argc > 1)
    {
        if (!cmdHandler.processArguments(rawArgs))
        {
            delete app;  // 清理资源
            return 0;
        }
        // 如果是查询命令，处理后直接退出
        if (cmdHandler.shouldExitAfterProcessing())
        {
            delete app;  // 清理资源
            return 0;
        }
        // 前台运行
        if(cmdHandler.isForegroundMode())
        {
            App a;
            if (!a.run())
            {
                qCritical() << "无法启动监控服务";
                delete app;
                return 1;
            }
            // 运行事件循环
            return app->exec();
        }
        // 运行事件循环
        result = app->exec();

        // 清理资源
        delete app;
        return result;
    }
    else
    {
        //        if (!checkRoot())
        //        {
        //            qCritical() << "请使用root用户启动";
        //            delete app;
        //            return 1;
        //        }
        // 检查是否已在运行
        if (isAlreadyRunning("monitor"))
        {
            qCritical() << "程序已在运行中";
            delete app;
            return 1;
        }

        pid_t pid = fork();
        if (pid == -1)
        {
            // 清理资源
            delete app;
            return -1;
        }
        else if (pid > 0)  // 父进程直接退出
        {
            qInfo() << "Starting daemon process";
            delete app;
            return result;
        }
        else  // 子进程转换为守护进程
        {
            if (!daemonize())
            {
                qCritical() << "守护进程初始化失败";
                return 1;
            }
            qInfo() << "Daemon process started";
            app = new QCoreApplication(argc, argv);
            app->setApplicationName("monitor");
            app->setApplicationVersion("1.0");
            // 启动监控服务
            App a;
            if (!a.run())
            {
                qCritical() << "无法启动监控服务";
                delete app;
                return 1;
            }
            // 运行事件循环
            result = app->exec();
            delete app;
            return result;
        }
    }
}
