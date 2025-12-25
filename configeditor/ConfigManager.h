#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QDateTime>
#include <QMutex>

/**
 * @brief 单例配置管理类（负责INI文件的读写）
 */
class ConfigManager : public QObject
{
    Q_OBJECT
    // 禁止外部构造、拷贝、赋值
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

public:
    /**
     * @brief 获取单例实例
     * @return 唯一实例指针
     */
    static ConfigManager* getInstance();

    /**
     * @brief 写入XML加载配置到INI文件
     * @param filePath XML文件路径
     * @param loadSuccess 加载是否成功
     */
    void writeLastOpenDir(const QString& filePath, bool loadSuccess);

    /**
     * @brief 写入XML加载配置到INI文件
     * @param filePath XML文件路径
     * @param loadSuccess 加载是否成功
     */
    void writeLastOpenFile(const QString& filePath, bool loadSuccess);

    /**
     * @brief 读取XML加载配置
     * @param[out] lastFilePath 最近打开的文件路径
     * @param[out] lastLoadTime 最近加载时间
     * @param[out] loadSuccess 最近加载是否成功
     * @return 读取成功返回true
     */
    bool readLastOpenDir(QString& lastFilePath, QString& lastLoadTime, bool& loadSuccess);

    /**
     * @brief 读取XML加载配置
     * @param[out] lastFilePath 最近打开的文件路径
     * @param[out] lastLoadTime 最近加载时间
     * @param[out] loadSuccess 最近加载是否成功
     * @return 读取成功返回true
     */
    bool readLastOpenFile(QString& lastFilePath, QString& lastLoadTime, bool& loadSuccess);

    /**
     * @brief 读取最近打开的文件列表（最多保留5个）
     * @return 文件路径列表
     */
    QStringList readRecentFileList();

private:
    explicit ConfigManager(QObject* parent = nullptr);
    ~ConfigManager() = default;

    static ConfigManager* m_instance;       // 单例实例
    static QMutex m_mutex;                 // 线程安全锁
    QSettings* m_settings;                  // INI配置管理器
    const QString m_iniFilePath;            // INI文件路径
    const QString m_xmlConfigGroup = "XmlLoadConfig"; // XML配置分组名
};
#define pConfigMgr ConfigManager::getInstance()

#endif // CONFIGMANAGER_H
