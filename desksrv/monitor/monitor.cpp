#include "monitor.h"
#include <QDebug>
#include <QTime>
#include <QProcess>
#include "configparser.h"
#include "globaldefine.h"
#include "extprocess.h"
#include "multischeduledtaskmgr.h"

Monitor::Monitor(InputWatcher *inputWatcher, QObject *parent)
    : QObject(parent)
    , m_tolerance_minutes(ConfigParser::getInst().getConfig().toleranceMinutes)
    , m_tolerance_minutes_day(ConfigParser::getInst().getConfig().toleranceMinutesDay)
    , m_monitor_start_time_1(ConfigParser::getInst().getConfig().timeRanges[0].first)
    , m_monitor_end_time_1(ConfigParser::getInst().getConfig().timeRanges[0].second)
    , m_monitor_start_time_2(ConfigParser::getInst().getConfig().timeRanges[1].first)
    , m_monitor_end_time_2(ConfigParser::getInst().getConfig().timeRanges[1].second)
    , m_inputWatcher(inputWatcher)
{
    initialize();
    m_dataManager   = new DataManager(this);
    m_monitorTimer  = new QTimer(this);
    m_activityTimer = new QTimer(this);

    // 每1s检查一次时间窗口
    m_monitorTimer->setInterval(1 * 1000);
    connect(m_monitorTimer, &QTimer::timeout, this, &Monitor::slotCheckTimeWindow);

    // 每10秒检查一次活动累计
    m_activityTimer->setInterval(10 * 1000);
    connect(m_activityTimer, &QTimer::timeout, [this]() {
        if (m_isMonitoringActive && m_inputWatcher->isUserActiveInLastSeconds(10))
        {
            m_totalActivitySeconds += 10;
            m_hasValidActivity = true;
            if(m_totalActivitySeconds%600==0)
            {
                qDebug() << "[Monitor] "
                         << "累计活动时间:" << m_totalActivitySeconds << "秒";
            }
        }
    });

    // 连接用户活动信号
    connect(m_inputWatcher, &InputWatcher::userActivityDetected, this, &Monitor::onUserActivity);

//    setScheduledTask();
}

bool Monitor::start()
{
    m_monitorTimer->start();
    return true;
}

bool Monitor::stop()
{
    qDebug() << "[Monitor] "
             << "停止监控服务";

    m_monitorTimer->stop();
    m_activityTimer->stop();
    m_inputWatcher->stopWatching();

    return true;
}

void Monitor::slotCheckTimeWindow()
{
    QTime currentTime = QTime::currentTime();

    // 超出监测时间
    if (isOutOfTimeWindow(currentTime))
    {
        endOfMonitoringPeriod();
    }
    else
    {
        // 如果进入监控窗口且之前未激活
        if (!m_isMonitoringActive)
        {
            m_isMonitoringActive = true;
            m_activityTimer->start();
            m_monitorTimer->start();
            m_currentMonitoringDate = QDateTime::currentDateTime();
            qDebug() << "[Monitor] "
                     << "进入监控时间段，开始监控用户活动";
        }
        else if (m_totalActivitySeconds >= m_tolerance_minutes_day * 60)  // 累积活动时间达标，提前结束
        {
            qDebug() << "[Monitor] "
                     << "达到活动时间，提前结束监控";
            endOfMonitoringPeriod();
            ExtProcess::screenShoot();
        }
    }
}

void Monitor::onUserActivity(const QString &event)
{
    UNUSED(event)
    if (m_isMonitoringActive)
    {
        //        qDebug() << "检测到用户活动: " << event;
        // 活动累计由定时器处理，这里只做日志
    }
}

void Monitor::initialize()
{
    m_isMonitoringActive   = false;
    m_hasValidActivity     = false;
    m_totalActivitySeconds = 0;
}

void Monitor::setScheduledTask()
{

    MultiScheduledTaskMgr::getInstance().addTask("中午十二点",
                                                 []() {
                                                     QString msg = "午饭";
                                                     ExtProcess::simplePopup(msg);
                                                 },
                                                 {QTime(12, 0)});
    MultiScheduledTaskMgr::getInstance().addTask("下午六点",
                                                 []() {
                                                     QString msg = "下班";
                                                     ExtProcess::simplePopup(msg);
                                                 },
                                                 {QTime(18, 0)});
    MultiScheduledTaskMgr::getInstance().addTask("晚上九点",
                                                 []() {
                                                     QString msg = "9点";
                                                     ExtProcess::simplePopup(msg);
                                                 },
                                                 {QTime(21, 0)});
}

void Monitor::endOfMonitoringPeriod()
{
    m_activityTimer->stop();
    m_monitorTimer->stop();
    QDateTime next;
    qint64    msecsToStart = calculateMsecsToNextStart(next);
    QTimer::singleShot(msecsToStart, this, &Monitor::slotCheckTimeWindow);
    qDebug().noquote() << "[Monitor] 监控结束，总活动时间:" << m_totalActivitySeconds << "秒"
                       << QString("计时器将在 %1 启动").arg(next.toString("yyyy-MM-dd hh:mm:ss"));

    // 如果活动时间达到容错时间，记录该日期
    if (m_hasValidActivity && m_totalActivitySeconds >= m_tolerance_minutes_day * 60)
    {
        m_dataManager->recordUsageDate(m_currentMonitoringDate.date());
        qDebug() << "[Monitor] "
                 << "记录有效使用日期:" << m_currentMonitoringDate.date().toString("yyyy-MM-dd");
    }
    initialize();
}

bool Monitor::isOutOfTimeWindow(QTime time)
{
    int currentHour   = time.hour();
    int currentMinute = time.minute();
    // 将当前时间转换为小时+小数形式（如8点30分 = 8.5）
    double currentTime = currentHour + currentMinute / 60.0;
    bool   outOfTime   = true;

    // 获取当前是星期几（Qt中Qt::Monday=1, ..., Qt::Sunday=7）
    QDate currentDate = QDate::currentDate();
    int   dayOfWeek   = currentDate.dayOfWeek();
    bool  isWorkday   = (dayOfWeek >= Qt::Monday && dayOfWeek <= Qt::Friday);

    // 根据是否工作日选择对应的时间段
    double startTime = isWorkday ? m_monitor_start_time_1 : m_monitor_start_time_2;
    double endTime   = isWorkday ? m_monitor_end_time_1 : m_monitor_end_time_2;

    // 正常时间段（如8:30-18:00）
    if (startTime < endTime)
    {
        outOfTime = currentTime < startTime || currentTime >= endTime;
    }
    else  // 跨天时间段（如19:00-01:00）
    {
        outOfTime = currentTime >= endTime && currentTime < startTime;
    }
    return outOfTime;
}

qint64 Monitor::calculateMsecsToNextStart(QDateTime &next)
{
    QDateTime now       = QDateTime::currentDateTime();
    QDate     today     = now.date();
    int       dayOfWeek = today.dayOfWeek();
    bool      isWorkday = (dayOfWeek >= Qt::Monday && dayOfWeek <= Qt::Friday);

    // 根据工作日/非工作日选择对应的开始时间
    double startTimeValue = isWorkday ? m_monitor_start_time_1 : m_monitor_start_time_2;
    int    startHour      = static_cast<int>(startTimeValue);
    int    startMinute    = static_cast<int>((startTimeValue - startHour) * 60);

    QTime     startTime(startHour, startMinute, 0);
    QDateTime nextStart(today, startTime);

    // 如果今天的开始时间已过，则检查明天是否需要切换时间段
    if (nextStart <= now)
    {
        QDate tomorrow          = today.addDays(1);
        int   tomorrowWeekday   = tomorrow.dayOfWeek();
        bool  tomorrowIsWorkday = (tomorrowWeekday >= Qt::Monday && tomorrowWeekday <= Qt::Friday);

        // 如果明天是不同类型的日子（工作日/非工作日），使用明天对应的时间段
        if (isWorkday != tomorrowIsWorkday)
        {
            double tomorrowStartTime = tomorrowIsWorkday ? m_monitor_start_time_1 : m_monitor_start_time_2;
            startHour                = static_cast<int>(tomorrowStartTime);
            startMinute              = static_cast<int>((tomorrowStartTime - startHour) * 60);
            startTime.setHMS(startHour, startMinute, 0);
        }

        nextStart = QDateTime(tomorrow, startTime);
    }
    next = nextStart;
    return now.msecsTo(nextStart);
}

QString Monitor::convertMSecsToHMS(qint64 msecs)
{
    // 确保输入为非负值
    if (msecs < 0)
    {
        return "0小时0分0秒";
    }

    // 转换为总秒数（忽略毫秒部分）
    qint64 totalSeconds = msecs / 1000;

    // 计算小时、分钟、秒
    int hours   = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    // 格式化输出字符串
    return QString("%1小时%2分%3秒").arg(hours).arg(minutes).arg(seconds);
}
