// VersionManager.h
#ifndef VERSIONMANAGER_H
#define VERSIONMANAGER_H

#include <QObject>
#include <QJsonObject>


class VersionManager : public QObject
{
    Q_OBJECT
public:
    explicit VersionManager(QObject *parent = nullptr);

    // 读取版本配置文件
    QJsonObject readVersionConfig(const QString& configPath = "version.json");

    // 对比版本号（格式：x.y.z），判断是否需要更新
    bool needUpdate(const QJsonObject& versionInfo);

    // 版本号字符串转数字（用于对比，如1.1.0 → 10100）
    int versionStrToInt(const QString& versionStr);

private:
    // 默认版本配置（配置文件读取失败时使用）
    QJsonObject defaultVersionInfo();
};

#endif // VERSIONMANAGER_H
