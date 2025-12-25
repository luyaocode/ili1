#ifndef TOOLITEM_H
#define TOOLITEM_H

#include <QString>
#include <QVariant>
#include <QFileInfo>
#include "commontool/commontool.h"

enum class QuickType
{
    Tool,    // 应用程序
    Common,  // 普通文件/目录
    MaxType
};
struct QuickItem
{
    QuickType type;
    QString   name;      // 工具名称
    QString   path;      // 工具路径
    QString   shortcut;  // 快捷键
    bool      isValid;   // 是否存在且可执行文件
    bool      enable;    // 是否监听
    // 序列化/反序列化辅助函数
    QVariantMap toVariantMap() const
    {
        QVariantMap map;
        map["type"]     = int(type);
        map["name"]     = name;
        map["path"]     = path;
        map["shortcut"] = shortcut;
        map["enable"]   = enable;
        return map;
    }

    static QuickItem fromVariantMap(const QVariantMap &map)
    {
        QuickItem item;
        item.type     = QuickType(map["type"].toInt());
        item.name     = map["name"].toString();
        item.path     = map["path"].toString();
        item.shortcut = map["shortcut"].toString();
        item.isValid  = Commontool::checkPathExist(item.path);
        item.enable   = map["enable"].toBool();
        return item;
    }
};

#endif  // TOOLITEM_H
