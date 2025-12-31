// VersionManager.cpp
#include "VersionManager.h"
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

VersionManager::VersionManager(QObject *parent): QObject(parent)
{
}

// 读取版本配置文件
QJsonObject VersionManager::readVersionConfig(const QString &configPath)
{
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "[VersionManager] 版本配置文件打开失败:" << file.errorString();
        return defaultVersionInfo();
    }

    // 解析JSON
    QByteArray    jsonData = file.readAll();
    QJsonDocument jsonDoc  = QJsonDocument::fromJson(jsonData);
    file.close();  // 提前关闭文件，避免资源泄漏

    // 情况1：JSON是对象（兼容原有单对象格式）
    if (!jsonDoc.isNull() && jsonDoc.isObject())
    {
        return jsonDoc.object();
    }

    // 情况2：JSON是数组，取第一个元素（适配你的数组格式）
    if (!jsonDoc.isNull() && jsonDoc.isArray())
    {
        QJsonArray jsonArray = jsonDoc.array();
        // 数组非空则返回第一个对象，否则返回默认值
        if (!jsonArray.isEmpty() && jsonArray.first().isObject())
        {
            return jsonArray.first().toObject();
        }
        else
        {
            qWarning() << "[VersionManager] 版本配置文件数组为空或第一个元素非对象";
            return defaultVersionInfo();
        }
    }

    // 情况3：JSON格式错误（非对象/非数组）
    qWarning() << "[VersionManager] 版本配置文件格式错误";
    return defaultVersionInfo();
}

// 对比版本号，判断是否需要更新
bool VersionManager::needUpdate(const QJsonObject &versionInfo)
{
    QString currentVer = versionInfo["currentVersion"].toString();
    QString latestVer  = versionInfo["latestVersion"].toString();

    int currentInt = versionStrToInt(currentVer);
    int latestInt  = versionStrToInt(latestVer);

    return latestInt > currentInt;
}

// 版本号转数字（如1.0.0 → 10000，1.10.5 → 11005）
int VersionManager::versionStrToInt(const QString &versionStr)
{
    QStringList parts = versionStr.split(".");
    // 补全3段（如1.1 → 1.1.0）
    while (parts.size() < 3)
    {
        parts.append("0");
    }
    // 转数字（每段占2位，最大支持99.99.99）
    int verInt = 0;
    verInt += parts[0].toInt() * 10000;
    verInt += parts[1].toInt() * 100;
    verInt += parts[2].toInt();
    return verInt;
}

// 默认版本配置（容错）
QJsonObject VersionManager::defaultVersionInfo()
{
    QJsonObject defaultInfo;
    defaultInfo["currentVersion"] = "1.0.0";
    defaultInfo["latestVersion"]  = "1.0.0";
    defaultInfo["updateTitle"]    = "版本信息";
    defaultInfo["updateContent"]  = QJsonArray() << "暂无更新说明";
    defaultInfo["forceUpdate"]    = false;
    return defaultInfo;
}
