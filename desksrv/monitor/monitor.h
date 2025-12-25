#ifndef MONITOR_H
#define MONITOR_H

#include <QObject>
#include <QDateTime>
#include <QTimer>
#include "inputwatcher.h"
#include "datamanager.h"

class Monitor : public QObject
{
    Q_OBJECT
public:
    explicit Monitor(InputWatcher *inputWatcher, QObject *parent = nullptr);
    bool start();
    bool stop();

private slots:
    // 检查并启动定时器（如果在监控时间内）
    void slotCheckTimeWindow();
    void onUserActivity(const QString &event);

private:
    void initialize();
    void setScheduledTask();
    // 检查当前小时是否在监控时间窗口外
    bool isOutOfTimeWindow(QTime time);
    void endOfMonitoringPeriod();
    // 计算距离下次开始时间的毫秒数
    qint64  calculateMsecsToNextStart(QDateTime &next);
    QString convertMSecsToHMS(qint64 msecs);

private:
    // 状态变量
    bool      m_isMonitoringActive;
    bool      m_hasValidActivity;
    qint64    m_totalActivitySeconds;
    QDateTime m_currentMonitoringDate;

    // 监控配置
    int    m_tolerance_minutes;      // 短期时间段判定为有效的时间阈值（分钟），暂时没有用
    int    m_tolerance_minutes_day;  // 长期时间段判定为有效的时间阈值
    double m_monitor_start_time_1;
    double m_monitor_end_time_1;
    double m_monitor_start_time_2;
    double m_monitor_end_time_2;

    InputWatcher *m_inputWatcher;   // 输入监测器
    DataManager  *m_dataManager;    // 日期数据管理器
    QTimer       *m_monitorTimer;   // 长期活动监测计时器
    QTimer       *m_activityTimer;  // 短期活动监测计时器
};

#endif  // MONITOR_H
