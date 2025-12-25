#include "scheduleitem.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

QJsonObject ScheduleItem::toJson() const
{
    QJsonObject obj;
    obj["taskConfig"] = taskConfig.toJson();  // 嵌套序列化TaskConfig
    obj["path"]       = path;
    obj["isValid"]    = isValid;
    obj["enable"]     = enable;
    return obj;
}

ScheduleItem ScheduleItem::fromJson(const QJsonObject &obj)
{
    ScheduleItem item;
    item.taskConfig = TaskConfig::fromJson(obj["taskConfig"].toObject());  // 嵌套反序列化
    item.path       = obj["path"].toString();
    item.isValid    = obj["isValid"].toBool();
    item.enable     = obj["enable"].toBool();
    return item;
}
