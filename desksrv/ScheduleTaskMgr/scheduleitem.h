#ifndef SCHEDULEITEM_H
#define SCHEDULEITEM_H

#include <QString>
#include <QJsonObject>
#include "commontool/multischeduledtaskmgr.h"

struct ScheduleItem
{
    TaskConfig taskConfig;  // 计划配置
    QString    path;        // 计划脚本或执行程序执行路径+参数
    bool       isValid;     // 是否存在计划
    bool       enable;      // 是否启用

    QJsonObject toJson() const;
    static ScheduleItem fromJson(const QJsonObject &obj);
};

#endif  // SCHEDULEITEM_H
