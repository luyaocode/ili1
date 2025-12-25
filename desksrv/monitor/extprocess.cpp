#include "extprocess.h"
#include <QDebug>
#include <QTime>
#include <QTimer>
#include <QProcess>
#include "configparser.h"
#include "globaldefine.h"
ExtProcess::ExtProcess()
{
    // 提醒弹窗计时器
    checkReminderTime();
}

void ExtProcess::process(const QString &cmd, const QStringList &params)
{
    // 定义reminder程序路径
    QString reminderPath(cmd);

    // 使用QProcess启动外部程序
    QProcess *process = new QProcess;
    // 启动程序（若需要传递参数，可在start的第二个参数添加，如 QStringList() << "param1"）
    process->start(reminderPath, params);

    // 可选：监听程序执行状态（成功/失败）
    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus exitStatus) {
                if (exitStatus == QProcess::NormalExit && exitCode == 0)
                {
                    qDebug() << "[Reminder] " << cmd << params << "执行成功";
                }
                else
                {
                    qDebug() << "[Reminder] " << cmd << params << "执行失败，退出码：" << exitCode;
                }
                process->deleteLater();  // 释放资源
            });

    // 可选：监听启动失败（如程序不存在）
    connect(process, &QProcess::errorOccurred, [=](QProcess::ProcessError error) {
        UNUSED(error)
        qDebug() << "[Reminder] 启动失败：" << cmd << params << process->errorString();
        process->deleteLater();
    });
}

void ExtProcess::eyesProtect()
{
    QStringList params {"screensaver"};
    process(REMINDER_PATH, params);
}

void ExtProcess::simplePopup(const QString &msg)
{
    QStringList params {"cornerpopup", msg};
    process(REMINDER_PATH, params);
}

void ExtProcess::recordScreen(int seconds, const QString &filePath)
{
    QStringList params {filePath, QString::number(seconds)};
    process(SCREENRECORDER_PATH, params);
}

void ExtProcess::screenShoot()
{
    QStringList params {"screenshoot"};
    process(SCREENRECORDER_PATH, params);
}

void ExtProcess::checkReminderTime()
{
    QMutexLocker lock(&mutex);
    if (ConfigParser::getInst().getConfig().reminderTimes.isEmpty())
    {
        return;
    }

    // 1. 获取当前时间（转换为小时+小数形式，例如19:30 → 19.5）
    QDateTime now         = QDateTime::currentDateTime();
    QTime     currentTime = now.time();
    double    currentHour = currentTime.hour() + currentTime.minute() / 60.0 + currentTime.second() / 3600.0;

    // 2. 筛选reminderTimes中所有未来的时间点（大于当前时间）
    QList<double> futureTimes;
    foreach (double time, ConfigParser::getInst().getConfig().reminderTimes)
    {
        if (time > currentHour)
        {
            futureTimes.append(time);
        }
    }

    // 3. 确定下一次提醒时间（若没有未来时间，则取次日第一个时间点）
    double nextReminderTime;
    if (!futureTimes.isEmpty())
    {
        // 从未来时间中取最小的（最近的）
        nextReminderTime = *std::min_element(futureTimes.begin(), futureTimes.end());
    }
    else
    {
        // 所有时间已过，取次日第一个时间点（默认按原列表顺序的第一个，或可排序后取最小）
        // 这里先按原列表第一个时间点处理，若需严格最小可先排序
        nextReminderTime = ConfigParser::getInst().getConfig().reminderTimes.first() + 24;  // +24表示次日
    }

    // 4. 计算当前时间到下一次提醒的间隔（单位：毫秒）
    double hourDiff = nextReminderTime - currentHour;
    // 处理跨天情况（例如当前23点，下一次是次日1点，hourDiff为2）
    int msecsToWait = static_cast<int>(hourDiff * 3600 * 1000);  // 小时→秒：1小时=3600秒

    // 5. 设置单次定时器，到时间后触发当前函数（形成循环检测）,延迟10ms，避免msecsToWait为0时高爆发
    QTimer::singleShot(msecsToWait, this, [this]() {
        // 第一步：触发提醒程序
        process(REMINDER_PATH);
        // 第二步：立即计算下一次提醒时间（形成循环）
        checkReminderTime();
    });

    // 调试输出
    QDateTime future = now.addMSecs(msecsToWait);
    qDebug().noquote() << "[Reminder] 下一次弹窗时间：" << future.toString("yyyy-MM-dd hh:mm:ss");
}
