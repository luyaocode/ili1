#include "datamanager.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QStringList>
#include "extprocess.h"

DataManager::DataManager(QObject *parent): QObject(parent)
{
    // 设置日志文件路径：~/.local/share/monitor/usage.log
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    m_logFilePath        = dataDir + "/usage.sh";
    m_activeTimeFilePath = dataDir + "/active.sh";
    // 确保日志文件存在
    ensureLogFileExists(m_logFilePath);
    ensureLogFileExists(m_activeTimeFilePath);
}

QString DataManager::getUsageLogFilePath()
{
    return m_logFilePath;
}

bool DataManager::ensureLogFileExists(const QString &filePath)
{
    QFile file(filePath);
    if (!file.exists())
    {
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            qCritical() << "[DataManager] 无法创建日志文件:" << filePath;
            return false;
        }
        file.close();
    }
    return true;
}

void DataManager::recordUsageDate(const QDate &date)
{
    if (!ensureLogFileExists(m_logFilePath))
    {
        return;
    }

    // 检查该日期是否已记录
    QList<QDate> existingDates = getUsageDates();
    if (existingDates.contains(date))
    {
        qDebug() << "[DataManager] 日期已记录，无需重复添加:" << date.toString();
        return;
    }

    // 追加记录新日期
    QFile file(m_logFilePath);
    if (file.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream out(&file);
        out << date.toString("yyyy-MM-dd") << "\n";
        file.close();
        qDebug() << "[DataManager] 已记录使用日期:" << date.toString();
        ExtProcess::simplePopup(date.toString("yyyy-MM-dd ") + "已记录!");
    }
    else
    {
        qCritical() << "[DataManager] 无法打开日志文件进行写入:" << m_logFilePath;
    }
}

QDate DataManager::parseDateFromString(const QString &dateStr)
{
    return QDate::fromString(dateStr.trimmed(), "yyyy-MM-dd");
}

QPair<QDate, int> DataManager::parseActiveTimeFromString(const QString &strline)
{
    QStringList strlist = strline.split(':');
    if (strlist.size() != 2)
    {
        return {};
    }
    return {parseDateFromString(strlist[0]), strlist[1].toInt()};
}

QList<QDate> DataManager::getUsageDates(int year, int month)
{
    QList<QDate> result;

    if (!ensureLogFileExists(m_logFilePath))
    {
        return result;
    }

    QFile file(m_logFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        QString     line;
        while ((line = in.readLine()) != nullptr)
        {
            QDate date = parseDateFromString(line);
            bool  flag = date.isValid();
            if (year > 0)
            {
                flag &= date.year() == year;
            }
            if (month > 0)
            {
                flag &= date.month() == month;
            }
            if (flag)
            {
                result.append(date);
            }
        }
        file.close();
    }
    else
    {
        qCritical() << "[DataManager] 无法打开日志文件进行读取:" << m_logFilePath;
    }

    // 排序并去重
    std::sort(result.begin(), result.end());
    auto last = std::unique(result.begin(), result.end());
    result.erase(last, result.end());

    return result;
}

void DataManager::recordActiveTime(const QDate &date, int duration)
{
    if (!ensureLogFileExists(m_activeTimeFilePath))
    {
        return;
    }

    // 检查该日期是否已记录
    QList<QPair<QDate, int>> existingDates = getActiveTime();
    bool                     contain       = false;
    for (const auto &pair : existingDates)
    {
        if (pair.first == date)
        {
            contain = true;
        }
    }
    if (contain)
    {
        qDebug() << "[DataManager] 活动时间已有记录，放弃更新:" << date.toString("yyyy-MM-dd");
        return;
    }

    // 追加记录新日期
    QFile file(m_activeTimeFilePath);
    if (file.open(QIODevice::Append | QIODevice::Text))
    {
        QTextStream out(&file);
        out << date.toString("yyyy-MM-dd") << ":" << QString::number(duration) << "\n";
        file.close();
        qDebug() << "[DataManager] 已记录活动时间:" << date.toString("yyyy-MM-dd") << ":" << QString::number(duration);
        ExtProcess::simplePopup(date.toString("yyyy-MM-dd：") + QString::number(duration) + " 已记录!");
    }
    else
    {
        qCritical() << "[DataManager] 无法打开活动时间文件进行写入:" << m_activeTimeFilePath;
    }
}

QList<QPair<QDate, int>> DataManager::getActiveTime(int year, int month)
{
    QList<QPair<QDate, int>> result;

    if (!ensureLogFileExists(m_activeTimeFilePath))
    {
        return result;
    }

    QFile file(m_activeTimeFilePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QTextStream in(&file);
        QString     line;
        while ((line = in.readLine()) != nullptr)
        {
            QPair<QDate, int> datePair   = parseActiveTimeFromString(line);
            QDate             date       = datePair.first;
            int               activeTime = datePair.second;
            bool              flag       = date.isValid();
            if ((activeTime < 0) || !flag)
            {
                continue;
            }
            if (year > 0)
            {
                flag &= date.year() == year;
            }
            if (month > 0)
            {
                flag &= date.month() == month;
            }
            if (flag)
            {
                result.append(datePair);
            }
        }
        file.close();
    }
    else
    {
        qCritical() << "[DataManager] 无法打开日志文件进行读取:" << m_activeTimeFilePath;
    }

    // 排序并去重
    std::sort(result.begin(), result.end());
    auto last = std::unique(result.begin(), result.end());
    result.erase(last, result.end());

    return result;
}
