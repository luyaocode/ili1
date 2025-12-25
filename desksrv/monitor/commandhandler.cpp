#include "commandhandler.h"
#include <QDebug>
#include <QDate>
#include <QCoreApplication>
#include "datamanager.h"
#include "guimanager.h"

CommandHandler::CommandHandler(): m_shouldExit(false), m_foregroundMode(false)
{
}

bool CommandHandler::processArguments(const QStringList &args)
{
    if (args.contains("--help") || args.contains("-h"))
    {
        showHelp();
        m_shouldExit = true;
        return true;
    }

    if (args.contains("--version") || args.contains("-v"))
    {
        showVersion();
        m_shouldExit = true;
        return true;
    }

    if (args.contains("--query") || args.contains("-q"))
    {
        bool showGui = args.contains("--gui") || args.contains("-g");
        queryUsage(showGui);
        m_shouldExit = !showGui;  // 如果显示GUI则不退出，否则退出
        return true;
    }

    if (args.contains("--foreground") || args.contains("-f"))
    {
        m_foregroundMode = true;
        return true;
    }
    showUsage();
    return false;
}

bool CommandHandler::shouldExitAfterProcessing() const
{
    return m_shouldExit;
}

bool CommandHandler::isForegroundMode() const
{
    return m_foregroundMode;
}

bool CommandHandler::needGui(const QStringList &args) const
{
    if (args.contains("--query") || args.contains("-q"))
    {
        bool showGui = args.contains("--gui") || args.contains("-g");
        return showGui;
    }
    return false;
}

void CommandHandler::showHelp()
{
    qInfo() << "monitor - 监控指定时间段内的电脑使用情况";
    qInfo() << "用法:";
    qInfo() << "  monitor [选项]";
    qInfo() << "选项:";
    qInfo() << "  -h, --help      显示帮助信息";
    qInfo() << "  -v, --version   显示版本信息";
    qInfo() << "  -q, --query     查询本月使用记录";
    qInfo() << "  -g, --gui       配合--query使用，以图形界面显示结果";
    qInfo() << "  -f, --foreground 前台运行，不进入后台";
}

void CommandHandler::showVersion()
{
    qInfo() << "monitor 版本" << QCoreApplication::applicationVersion();
}

void CommandHandler::showUsage()
{
    // 使用QTextStream输出，确保跨平台兼容性
    QTextStream out(stdout);

    // 程序名称和基本用法
    out << "Usage: monitor [options]" << endl << endl;

    // 选项列表
    out << "Options:" << endl;
    out << "  -h, --help           Show this help message and exit" << endl;
    out << "  -v, --version        Show version information and exit" << endl;
    out << "  -q, --query          Perform a query operation" << endl;
    out << "  -g, --gui            Show GUI when used with --query" << endl;
    out << "  -f, --foreground     Run in foreground mode (don't daemonize)" << endl << endl;

    // 使用示例
    out << "Examples:" << endl;
    out << "  monitor --query        Perform a query in command-line mode" << endl;
    out << "  monitor --query --gui  Perform a query with GUI interface" << endl;
    out << "  monitor --foreground   Run the application in foreground" << endl;
}

QList<QDate> CommandHandler::getCurrentMonthUsageDates()
{
    DataManager dataManager;
    QDate       today = QDate::currentDate();
    return dataManager.getUsageDates(today.year(), today.month());
}
QList<QDate> CommandHandler::getCurrentYearUsageDates()
{
    DataManager dataManager;
    QDate       today = QDate::currentDate();
    return dataManager.getUsageDates(today.year());
}
QList<QDate> CommandHandler::getTotalUsageDates()
{
    DataManager dataManager;
    return dataManager.getUsageDates();
}

void CommandHandler::queryUsage(bool showGui)
{
    QList<QDate> monthUsageDates = getCurrentMonthUsageDates();
    QList<QDate> yearUsageDates  = getCurrentYearUsageDates();
    QList<QDate> totalUsageDates = getTotalUsageDates();

    if (showGui)
    {
        // 显示图形界面
        GuiManager::getInst().showCalendar(totalUsageDates);
    }
    else
    {
        // 命令行输出
        if (monthUsageDates.isEmpty())
        {
            qInfo() << "本月设定时间段内没有记录到电脑使用";
            return;
        }

        qInfo() << "=======设定时间段内电脑使用的日期=======";
        for (const QDate &date : monthUsageDates)
        {
            qInfo() << "  " << date.toString("yyyy-MM-dd");
        }
        qInfo() << "本月总计:" << monthUsageDates.size() << "天";
        qInfo() << "本年总计:" << yearUsageDates.size() << "天";
        qInfo() << "全部总计:" << totalUsageDates.size() << "天";

        DataManager dataMgr;
        QDate       today          = QDate::currentDate();
        auto        activeTimeList = dataMgr.getActiveTime(today.year(), today.month());
        qInfo() << "=======本月电脑每日使用时长=======";
        for (const auto &pair : activeTimeList)
        {
            float hour = float(pair.second) / 3600;
            qInfo().noquote() << pair.first.toString("MM-dd:") << QString::number(hour,'f',2) << "小时";
        }
        qInfo() << "=======本年电脑每日使用时长=======";
        activeTimeList = dataMgr.getActiveTime(today.year());
        for (const auto &pair : activeTimeList)
        {
            float hour = float(pair.second) / 3600;
            qInfo().noquote() << pair.first.toString("MM-dd:") << QString::number(hour,'f',2) << "小时";
        }
        qInfo() << "=======全部电脑每日使用时长=======";
        activeTimeList = dataMgr.getActiveTime();
        for (const auto &pair : activeTimeList)
        {
            float hour = float(pair.second) / 3600;
            qInfo().noquote() << pair.first.toString("yyyy-MM-dd:") << QString::number(hour,'f',2) << "小时";
        }
    }
}
