#ifndef DATAMANAGER_H
#define DATAMANAGER_H

#include <QObject>
#include <QDate>
#include <QList>
#include <QPair>
#include <QString>

class DataManager : public QObject
{
    Q_OBJECT
public:
    explicit DataManager(QObject *parent = nullptr);
    void                     recordUsageDate(const QDate &date);                 // 写日期到文件
    QList<QDate>             getUsageDates(int year = 0, int month = 0);         // 从文件读日期
    void                     recordActiveTime(const QDate &date, int duration);  // 写活动时间，单位秒
    QList<QPair<QDate, int>> getActiveTime(int year = 0, int month = 0);         // 读活动时间
    QString                  getUsageLogFilePath();                              // 获取日期文件路径

private:
    bool              ensureLogFileExists(const QString &filePath);
    QDate             parseDateFromString(const QString &dateStr);
    QPair<QDate, int> parseActiveTimeFromString(const QString &strline);

private:
    QString m_logFilePath;         // 日期文件
    QString m_activeTimeFilePath;  // 活动时间文件
};

#endif  // DATAMANAGER_H
