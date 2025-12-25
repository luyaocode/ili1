#include "configparser.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>
#include <globaldefine.h>

MonitorConfig config;

bool ConfigParser::parseConfig(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "[ConfigParser] "
                   << "无法打开配置文件:" << filePath;
        return false;
    }

    QTextStream in(&file);
    // 修复正则表达式，确保能匹配整数和小数
    QRegularExpression timeRangeRegex("^monitor_(start|end)_(\\d+)=(\\d+(\\.\\d+)?)$");
    QRegularExpression toleranceRegex("^tolerance_minutes=(\\d+)$");
    QRegularExpression toleranceDayRegex("^tolerance_minutes_day=(\\d+)$");
    QRegularExpression logPathRegex("^log_path=(.*)$");
    QRegularExpression mouseRegex("^monitor_mouse=(true|false)$");
    QRegularExpression keyboardRegex("^monitor_keyboard=(true|false)$");

    QRegularExpression reminderTimesRegex("^reminder_time_(\\d+)=(\\d+(\\.\\d+)?)$");
    QRegularExpression eyesToleranceMinutes("^eyes_tolerance_minutes=(\\d+)$");

    // 用于临时存储时间段，键为时间段编号，值为开始和结束时间对
    QMap<int, QPair<double, double>> tempTimeRanges;

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();

        // 忽略注释和空行
        if (line.startsWith("#") || line.isEmpty())
        {
            continue;
        }

        // 解析时间段配置
        QRegularExpressionMatch match = timeRangeRegex.match(line);
        if (match.hasMatch())
        {
            QString type     = match.captured(1);             // "start" 或 "end"
            int     rangeNum = match.captured(2).toInt();     // 时间段编号
            double  time     = match.captured(3).toDouble();  // 时间值

            // 初始化该编号的时间段条目
            if (!tempTimeRanges.contains(rangeNum))
            {
                tempTimeRanges[rangeNum] = QPair<double, double>(-1, -1);
            }

            // 设置开始或结束时间
            if (type == "start")
            {
                tempTimeRanges[rangeNum].first = time;
                qDebug() << "[ConfigParser] "
                         << "解析到开始时间：" << rangeNum << "=" << time;
            }
            else if (type == "end")
            {
                tempTimeRanges[rangeNum].second = time;
                qDebug() << "[ConfigParser] "
                         << "解析到结束时间：" << rangeNum << "=" << time;
            }
            continue;
        }

        // 解析普通容错时间
        match = toleranceRegex.match(line);
        if (match.hasMatch())
        {
            config.toleranceMinutes = match.captured(1).toInt();
            qDebug() << "[ConfigParser] "
                     << "解析到普通容错时间：" << QString::number(config.toleranceMinutes);
            continue;
        }

        // 解析每日容错时间
        match = toleranceDayRegex.match(line);
        if (match.hasMatch())
        {
            config.toleranceMinutesDay = match.captured(1).toInt();
            qDebug() << "[ConfigParser] "
                     << "解析到每日容错时间：" << QString::number(config.toleranceMinutesDay);
            continue;
        }

        // 解析日志文件路径
        match = logPathRegex.match(line);
        if (match.hasMatch())
        {
            config.logPath = match.captured(1);
            qDebug() << "[ConfigParser] "
                     << "解析到日志文件路径：" << config.logPath;
            continue;
        }

        // 解析鼠标监控配置
        match = mouseRegex.match(line);
        if (match.hasMatch())
        {
            config.monitorMouse = (match.captured(1) == "true");
            qDebug() << "[ConfigParser] "
                     << "解析到鼠标监控配置：" << config.monitorMouse;
            continue;
        }

        // 解析键盘监控配置
        match = keyboardRegex.match(line);
        if (match.hasMatch())
        {
            config.monitorKeyboard = (match.captured(1) == "true");
            qDebug() << "[ConfigParser] "
                     << "解析到键盘监控配置：" << config.monitorKeyboard;
            continue;
        }

        // 解析reminder时间点配置
        match = reminderTimesRegex.match(line);
        if (match.hasMatch())
        {
            QString timeStr = match.captured(2);   // 提取等号后的数值字符串
            double  timeVal = timeStr.toDouble();  // 转换为double
            config.reminderTimes.append(timeVal);  // 存入列表
            qDebug() << "[ConfigParser] "
                     << "解析到弹窗时间：" << timeVal;
            continue;
        }

        match = eyesToleranceMinutes.match(line);
        if (match.hasMatch())
        {
            config.eyes_tolerance_minutes = match.captured(1).toInt();
            qDebug() << "[ConfigParser] "
                     << "解析到屏保时间：" << QString::number(config.eyes_tolerance_minutes);
            continue;
        }

        // 未知配置项
        qWarning() << "[ConfigParser] "
                   << "未知的配置项：" << line;
    }

    file.close();

    // 将临时存储的时间段整理到配置中，过滤无效的时间段
    for (auto it = tempTimeRanges.begin(); it != tempTimeRanges.end(); ++it)
    {
        if (it.value().first != -1 && it.value().second != -1)
        {
            config.timeRanges.append(it.value());
            qDebug() << "[ConfigParser] "
                     << "添加时间段：" << it.value().first << "到" << it.value().second;
        }
        else
        {
            qWarning() << "[ConfigParser] "
                       << "时间段" << it.key() << "配置不完整，已忽略";
        }
    }

    return true;
}

ConfigParser &ConfigParser::getInst()
{
    static ConfigParser inst;
    return inst;
}

MonitorConfig &ConfigParser::getConfig()
{
    return config;
}

ConfigParser::ConfigParser()
{
    qDebug() << "[ConfigParser] 解析配置" << CONFIG_PATH;
    parseConfig(CONFIG_PATH);
}

ConfigParser::~ConfigParser()
{
}
