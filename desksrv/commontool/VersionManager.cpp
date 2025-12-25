// VersionManager.cpp
#include "VersionManager.h"
#include <QJsonArray>
#include <QFile>
#include <QJsonDocument>
#include <QDebug>

VersionManager::VersionManager(QObject *parent)
    : QObject(parent)
{}

// 读取版本配置文件
QJsonObject VersionManager::readVersionConfig(const QString& configPath)
{
    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "[VersionManager] 版本配置文件读取失败：" << file.errorString();
        return defaultVersionInfo(); // 返回默认配置
    }

    // 解析JSON
    QByteArray jsonData = file.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        qWarning() << "[VersionManager] 版本配置文件格式错误";
        return defaultVersionInfo();
    }

    file.close();
    return jsonDoc.object();
}

// 对比版本号，判断是否需要更新
bool VersionManager::needUpdate(const QJsonObject& versionInfo)
{
    QString currentVer = versionInfo["currentVersion"].toString();
    QString latestVer = versionInfo["latestVersion"].toString();

    int currentInt = versionStrToInt(currentVer);
    int latestInt = versionStrToInt(latestVer);

    return latestInt > currentInt;
}

// 版本号转数字（如1.0.0 → 10000，1.10.5 → 11005）
int VersionManager::versionStrToInt(const QString& versionStr)
{
    QStringList parts = versionStr.split(".");
    // 补全3段（如1.1 → 1.1.0）
    while (parts.size() < 3) {
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
    defaultInfo["latestVersion"] = "1.0.0";
    defaultInfo["updateTitle"] = "版本信息";
    defaultInfo["updateContent"] = QJsonArray() << "暂无更新说明";
    defaultInfo["forceUpdate"] = false;
    return defaultInfo;
}
