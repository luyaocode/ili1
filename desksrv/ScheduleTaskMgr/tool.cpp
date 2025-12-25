#include "tool.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <functional>

#include "scheduleitem.h"

void createConfigFile(const QString &filePath)
{
    QFile file(filePath);
    if (file.exists())
    {
        return;
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Failed to create new config file:" << filePath;
        return;
    }
    QJsonArray array;

    //
    QList<ScheduleItem> list;
    ScheduleItem        item1;
    item1.enable                = true;
    item1.path                  = "/usr/bin/ScreenRecorder11";
    item1.taskConfig.taskName   = "打开录屏工具";
    item1.taskConfig.type       = TaskType::TimePoint;
    item1.taskConfig.timePoints = QList<QTime> {QTime(18,00)};
    list.push_back(item1);

    ScheduleItem        item2;
    item2.enable                = true;
    item2.path                  = "reminder cornerpopup 1h...";
    item2.taskConfig.taskName   = "cornerpopup";
    item2.taskConfig.type       = TaskType::Interval;
    item2.taskConfig.intervalSec = 3600;
    list.push_back(item2);

    for (auto &item : list)
    {
        QJsonObject obj = item.toJson();
        array.append(obj);
    }

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
    qDebug() << "Config file created: " << filePath;
}

QString taskTypeToString(TaskType type)
{
    switch (type)
    {
        case TaskType::Interval:
            return "间隔一段时间执行";
        case TaskType::TimePoint:
            return "到达时间点执行";
        default:
            break;
    }
    return "";
}

QString taskStatusToString(bool status)
{
    return status ? "启用" : "禁用";
}

QString triggerTimeToString(const QList<QTime> list)
{
    QString info;
    for (auto &time : list)
    {
        info += time.toString("HH:mm:ss") + ";";
    }
    auto idx = info.lastIndexOf(';');
    info     = info.mid(0, idx);
    return info;
}

QString triggerTimeToString(const int interval)
{
    return QString::number(interval);
}
