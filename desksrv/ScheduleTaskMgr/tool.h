#ifndef TOOL_H
#define TOOL_H
#include <QString>
#include "commontool/multischeduledtaskmgr.h"

void    createConfigFile(const QString &filePath);
QString taskTypeToString(TaskType type);
QString taskStatusToString(bool status);
QString triggerTimeToString(const QList<QTime> list);
QString triggerTimeToString(const int interval);
#endif  // TOOL_H
