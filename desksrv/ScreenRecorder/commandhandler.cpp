#include "commandhandler.h"

#include <QDebug>
#include <QTextStream>
#include <QCoreApplication>
#include <QTimer>

#include "mainwindow.h"
#include "screenrecorder.h"

// 构造函数：初始化应用引用和成员变量
CommandHandler::CommandHandler(QApplication &app)
    : m_app(app), m_exit(true), m_command(""), m_filepath(""), m_duration(0)
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

    if (args.size() > 1)
    {
        if (args.size() == 2)
        {
            bool ok    = false;
            m_duration = args[1].toInt(&ok);
            if (!ok)
            {
                m_duration  = 0;
                QString cmd = args[1];
                if (cmd == "screenshoot")
                {
                    ScreenRecorder recorder;
                    recorder.screenShoot();
                    if (!recorder.lastError().isEmpty())
                    {
                        qDebug().noquote() << "[ScreenRecorder] " << recorder.lastError();
                    }
                    m_exit = true;
                    return true;
                }
            }
        }
        if (args.size() == 3)
        {
            m_filepath = args[2];
        }
        if (m_duration > 0)
        {
            m_exit    = false;
            m_command = "record";
            return true;
        }
    }

    // 无参数
    if (args.size() == 1)
    {
        m_exit    = false;
        m_command = "ui";
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
        return 0;
    }

    // 根据解析的命令执行不同窗口
    if (m_command == "ui")
    {
        MainWindow window;
        window.show();
        return m_app.exec();
    }
    else if (m_command == "record")
    {
        ScreenRecorder recorder;
        recorder.startRecording(m_filepath);
        if (!recorder.lastError().isEmpty())
        {
            qDebug().noquote() << "[ScreenRecorder] " << recorder.lastError();
        }
        QTimer::singleShot(m_duration * 1000, &recorder, [&]() {
            recorder.stopRecording();
            qApp->quit();
        });
        return m_app.exec();
    }

    return 0;
}

// 以下是原有辅助方法的实现
void CommandHandler::showHelp()
{
    qInfo() << "ScreenRecorder - 录屏工具";
    qInfo() << "用法:";
    qInfo() << "  ScreenRecorder [选项]";
    qInfo() << "选项:";
    qInfo() << "  -h, --help               显示帮助信息";
    qInfo() << "  -v, --version            显示版本信息";
    qInfo() << "  [duration] [filepath]    直接录屏";
    qInfo() << "  screenshoot              截屏";
}

void CommandHandler::showVersion()
{
    qInfo() << QCoreApplication::applicationName() << " 版本: " << QCoreApplication::applicationVersion();
}

void CommandHandler::showUsage()
{
    QTextStream out(stdout);
    out << "Usage: ScreenRecorder [options]" << endl << endl;
    out << "Options:" << endl;
    out << "  -h, --help                Show this help message and exit" << endl;
    out << "  -v, --version             Show version information and exit" << endl;
    out << "  [duration] [filepath]     Record and save file" << endl;
    out << "  screenshoot               Screenshoot" << endl;
}
