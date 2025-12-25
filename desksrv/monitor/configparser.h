#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <QString>
#include <QList>
#include <QPair>

struct MonitorConfig
{
    // 时间段列表，存储开始和结束时间对
    QList<QPair<double, double>> timeRanges;

    // 容错时间
    int toleranceMinutes;
    int toleranceMinutesDay;

    // 日志文件路径
    QString logPath;

    // 输入设备监控配置
    bool monitorMouse;
    bool monitorKeyboard;

    // 弹窗时间点
    QList<double> reminderTimes;

    // eyesprotector容忍时间
    int eyes_tolerance_minutes;

    // 构造函数初始化默认值
    MonitorConfig(): toleranceMinutes(0), toleranceMinutesDay(0), monitorMouse(false), monitorKeyboard(false)
    {
    }
};

class ConfigParser
{
public:
    static ConfigParser         &getInst();
    QList<QPair<double, double>> getTimeRanges() const;
    MonitorConfig               &getConfig();

private:
    explicit ConfigParser();
    ~ConfigParser();
    bool parseConfig(const QString &filePath);
};

#endif  // CONFIGPARSER_H
