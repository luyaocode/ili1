#include "eyesprotector.h"
#include <QDebug>

#include "configparser.h"
#include "app.h"
#include "globaldefine.h"
#include "extprocess.h"
#include "multischeduledtaskmgr.h"

EyesProtector::EyesProtector(InputWatcher *inputWatcher, QObject *parent)
    : QObject(parent)
    , eyes_tolerance_minutes(ConfigParser::getInst().getConfig().eyes_tolerance_minutes)
    , m_inputWatcher(inputWatcher)
    , m_activityTimer(new QTimer(this))
    , m_activitySeconds(0)
    , m_totalActivitySecondsToday(0)
{
    // 每10秒检查一次活动累计
    m_activityTimer->setInterval(10 * 1000);
    connect(m_activityTimer, &QTimer::timeout, this, [=]() {
        if (m_inputWatcher->isUserActiveInLastSeconds(10))
        {
            m_totalActivitySecondsToday += 10;
            m_activitySeconds += 10;
            if (m_totalActivitySecondsToday % 600 == 0)
            {
                qDebug() << "[EyesProtector] "
                         << "累计活动时间:" << m_activitySeconds << "秒";
            }
            if (eyes_tolerance_minutes != 0 && eyes_tolerance_minutes * 60 == m_activitySeconds)
            {
                ExtProcess::eyesProtect();
                m_activitySeconds = 0;
            }
        }
    });

    // 连接用户活动信号
    connect(m_inputWatcher, &InputWatcher::userActivityDetected, this, &EyesProtector::onUserActivity);

    setScheduledTask();
}

bool EyesProtector::start()
{
    m_activityTimer->start();
    return true;
}

void EyesProtector::onUserActivity(const QString &event)
{
    UNUSED(event)
}

void EyesProtector::setScheduledTask()
{
    // 统计累积活动时间
    MultiScheduledTaskMgr::getInstance().addTask("统计全天活动时间",
                                                 [this]() {
                                                     DataManager mgr;
                                                     mgr.recordActiveTime(QDate::currentDate(),
                                                                          m_totalActivitySecondsToday);
                                                     m_totalActivitySecondsToday = 0;
                                                 },
                                                 {QTime(23, 59)});
    // 清除累积活动时间
    MultiScheduledTaskMgr::getInstance().addTask("清除累积活动时间",
                                                 [this]() {
                                                     m_activitySeconds           = 0;
                                                     m_totalActivitySecondsToday = 0;
                                                 },
                                                 {QTime(23, 59)});
}
