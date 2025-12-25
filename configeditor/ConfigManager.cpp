#include "ConfigManager.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDebug>

// 初始化静态成员
ConfigManager* ConfigManager::m_instance = nullptr;
QMutex ConfigManager::m_mutex;

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
    // 初始化INI文件路径（程序运行目录下的 config.ini）
    , m_iniFilePath(QCoreApplication::applicationDirPath() + "/config.ini")
{
    // 初始化QSettings（INI格式 + UTF-8编码）
    m_settings = new QSettings(m_iniFilePath, QSettings::IniFormat, this);
    m_settings->setIniCodec("UTF-8");

    qInfo() << "[ConfigManager] INI配置文件路径：" << m_iniFilePath;
}

ConfigManager* ConfigManager::getInstance()
{
    // 双重检查锁定（DCLP），保证线程安全且高效
    if (!m_instance) {
        QMutexLocker locker(&m_mutex);
        if (!m_instance) {
            m_instance = new ConfigManager();
        }
    }
    return m_instance;
}

void ConfigManager::writeLastOpenDir(const QString& filePath, bool loadSuccess)
{
    if(!QFileInfo(filePath).isDir())
        return;
    QMutexLocker locker(&m_mutex); // 线程安全写入
    m_settings->beginGroup(m_xmlConfigGroup);

    // 基础配置
    m_settings->setValue("LastOpenFilePath", filePath);
    m_settings->setValue("LastLoadTime", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    m_settings->setValue("LoadSuccess", loadSuccess);
    m_settings->setValue("LoadResult", loadSuccess ? "加载成功" : "加载失败");

    m_settings->endGroup();
    m_settings->sync(); // 强制写入磁盘

    qInfo() << "[ConfigManager] 打开目录：" << filePath;
}

void ConfigManager::writeLastOpenFile(const QString &filePath, bool loadSuccess)
{
    if(QFileInfo(filePath).isDir())
        return;
    QStringList recentFiles;
    {
        // 最近打开文件列表（去重 + 保留最近5个）
        recentFiles = readRecentFileList();
        recentFiles.removeAll(filePath); // 去重
        recentFiles.prepend(filePath);   // 添加到列表头部
        if (recentFiles.size() > 5) {
            recentFiles.takeLast(); // 只保留最近5个
        }
    }
    QMutexLocker locker(&m_mutex); // 线程安全写入
    m_settings->beginGroup(m_xmlConfigGroup);
    m_settings->setValue("LastOpenFileName", QFileInfo(filePath).fileName());
    m_settings->setValue("RecentFileList", recentFiles);
    m_settings->endGroup();
    m_settings->sync(); // 强制写入磁盘

    qInfo() << "[ConfigManager] 打开文件：" << filePath;
}

bool ConfigManager::readLastOpenDir(QString& lastFilePath, QString& lastLoadTime, bool& loadSuccess)
{
    QMutexLocker locker(&m_mutex); // 线程安全读取
    if (!m_settings->contains(m_xmlConfigGroup + "/LastOpenFilePath")) {
        qWarning() << "[ConfigManager] 未找到XML加载配置";
        return false;
    }

    m_settings->beginGroup(m_xmlConfigGroup);
    lastFilePath = m_settings->value("LastOpenFilePath").toString();
    lastLoadTime = m_settings->value("LastLoadTime").toString();
    loadSuccess = m_settings->value("LoadSuccess").toBool();
    m_settings->endGroup();

    return true;
}

bool ConfigManager::readLastOpenFile(QString &lastFilePath, QString &lastLoadTime, bool &loadSuccess)
{
    QStringList recentFiles = readRecentFileList();
    lastFilePath = recentFiles.first();
    return !lastFilePath.isEmpty();
}

QStringList ConfigManager::readRecentFileList()
{
    QMutexLocker locker(&m_mutex);
    m_settings->beginGroup(m_xmlConfigGroup);
    QStringList recentFiles = m_settings->value("RecentFileList").toStringList();
    m_settings->endGroup();
    return recentFiles;
}
