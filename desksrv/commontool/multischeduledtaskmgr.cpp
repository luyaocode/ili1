#include "multischeduledtaskmgr.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "commontool.h"

// 初始化静态成员
MultiScheduledTaskMgr *MultiScheduledTaskMgr::m_instance = nullptr;
std::mutex             MultiScheduledTaskMgr::m_mutex;

MultiScheduledTaskMgr &MultiScheduledTaskMgr::getInstance()
{
    // 双重检查锁：避免多线程同时创建实例
    if (!m_instance)
    {
        std::lock_guard<std::mutex> lock(m_mutex);  // 加锁保护
        if (!m_instance)
        {
            m_instance = new MultiScheduledTaskMgr();
        }
    }
    return *m_instance;
}

TaskID MultiScheduledTaskMgr::addTask(TaskConfig &config)
{
    // 校验任务合法性
    if (!validateTask(config))
    {
        qWarning() << "MultiScheduledTaskMgr 任务" << config.taskName << "添加失败！";
        return 0;
    }

    // 加锁保护任务列表（多线程写入安全）
    std::lock_guard<std::mutex> lock(m_taskMutex);
    auto                        taskId = generate_task_id();
    config.id                          = taskId;
    m_tasks.append(config);
    qDebug() << "[MultiScheduledTaskMgr] 任务添加成功：" << config.taskName << "taskID: " << taskId;
    return taskId;
}

bool MultiScheduledTaskMgr::removeTask(TaskID id)
{
    int idx = -1;
    for (int i = 0; i < m_tasks.size(); ++i)
    {
        if (m_tasks[i].id == id)
        {
            idx = i;
            break;
        }
    }
    if (idx == -1)
    {
        return false;
    }
    m_tasks.removeAt(idx);
    qDebug() << "[MultiScheduledTaskMgr] 任务移除成功："
             << "taskID: " << id;
    return true;
}

bool MultiScheduledTaskMgr::updateTask(const TaskConfig &config)
{
    int idx = -1;
    for (int i = 0; i < m_tasks.size(); ++i)
    {
        if (m_tasks[i].id == config.id)
        {
            idx = i;
            break;
        }
    }
    if (idx == -1)
    {
        return false;
    }
    m_tasks[idx] = config;
    qDebug() << "[MultiScheduledTaskMgr] 任务更新成功："
             << "taskID: " << config.id;
    return true;
}

TaskID MultiScheduledTaskMgr::addTask(const QString &taskName, std::function<void()> callback, int intervalSec)
{
    TaskConfig config;
    config.taskName    = taskName;
    config.type        = TaskType::Interval;
    config.timePoints  = {};
    config.intervalSec = intervalSec;
    config.callback    = std::move(callback);
    return addTask(config);
}

TaskID MultiScheduledTaskMgr::addTask(const QString        &taskName,
                                      std::function<void()> callback,
                                      const QList<QTime>   &timePoints)
{
    TaskConfig config;
    config.taskName    = taskName;
    config.type        = TaskType::TimePoint;
    config.timePoints  = timePoints;
    config.intervalSec = 0;
    config.callback    = std::move(callback);
    return addTask(config);
}

MultiScheduledTaskMgr::MultiScheduledTaskMgr(QObject *parent): QObject(parent), m_checkTimer(new QTimer(this))
{
    m_next_task_id  = 1;
    m_max_task_size = MAX_TASK_SIZE;
    // 每秒检查一次任务（可调整精度）
    m_checkTimer->setInterval(1000);
    connect(m_checkTimer, &QTimer::timeout, this, &MultiScheduledTaskMgr::checkTasks);
    m_checkTimer->start();
}

bool MultiScheduledTaskMgr::validateTask(const TaskConfig &config)
{
    if (config.callback == nullptr)
    {
        qWarning() << "[MultiScheduledTaskMgr] 任务" << config.taskName << "未设置回调函数";
        return false;
    }
    if (config.type == TaskType::TimePoint && config.timePoints.isEmpty())
    {
        qWarning() << "[MultiScheduledTaskMgr] 时间点任务" << config.taskName << "未设置时间点";
        return false;
    }
    if (config.type == TaskType::Interval && config.intervalSec <= 0)
    {
        qWarning() << "[MultiScheduledTaskMgr] 间隔任务" << config.taskName << "间隔时间无效（必须>0）";
        return false;
    }
    return true;
}

void MultiScheduledTaskMgr::checkTasks()
{
    QDateTime now = QDateTime::currentDateTime();
    // 加锁读取任务列表（避免读取时被修改）
    std::lock_guard<std::mutex> lock(m_taskMutex);
    for (const auto &task : m_tasks)
    {
        if (task.type == TaskType::TimePoint)
        {
            checkTimePointTask(task, now);
        }
        else
        {
            checkIntervalTask(task, now);
        }
    }
}

void MultiScheduledTaskMgr::checkTimePointTask(const TaskConfig &task, const QDateTime &now)
{
    QDate today       = now.date();
    QTime currentTime = now.time();  // 获取当前时间（时分秒）

    for (const QTime &tp : task.timePoints)
    {
        // 关键：当前时间的时分与时间点的时分完全匹配（忽略秒和毫秒，避免过于严苛）
        if (currentTime.hour() == tp.hour() && currentTime.minute() == tp.minute())
        {
            std::lock_guard<std::mutex> lock(m_lastRunMutex);
            QDateTime                   lastRun = m_lastRun.value(task.id, QDateTime());

            // 确保该时间点今天未触发过
            if (lastRun.date() != today || lastRun.time() < tp)
            {
                runTask(task);
                // 记录为当前时间点，确保当天该时间点仅触发一次
                m_lastRun[task.id] = QDateTime(today, tp);
                // 若同一时间点有多个，触发一次即可
                break;
            }
        }
    }
}

void MultiScheduledTaskMgr::checkIntervalTask(const TaskConfig &task, const QDateTime &now)
{
    // 加锁检查上次执行时间（多线程安全）
    std::lock_guard<std::mutex> lock(m_lastRunMutex);
    QDateTime                   lastRun = m_lastRun.value(task.id, QDateTime());
    // 首次不立即执行，后续间隔时间到则执行
    if (lastRun.isNull() || lastRun.secsTo(now) >= task.intervalSec)
    {
        if (!lastRun.isNull())
        {
            runTask(task);
        }
        m_lastRun[task.id] = now;
    }
}

void MultiScheduledTaskMgr::runTask(const TaskConfig &task)
{
    qDebug() << "[MultiScheduledTaskMgr] 执行任务：" << task.taskName;
    task.callback();
}

TaskID MultiScheduledTaskMgr::generate_task_id()
{
    return m_next_task_id++;
}

////////////////////////////////////辅助函数////////////////////////////////

QJsonObject timeToJson(const QTime &time)
{
    QJsonObject obj;
    obj["time"] = time.toString("HH:mm:ss");
    return obj;
}
QTime jsonToTime(const QJsonObject &obj)
{
    return QTime::fromString(obj["time"].toString(), "HH:mm:ss");
}
////////////////////////////////////////////////////////////////////

bool TaskConfig::operator==(const TaskConfig &config)
{
    bool equal = true;
    equal &= (taskName == config.taskName);
    equal &= (type == config.type);
    if (!equal)
    {
        return false;
    }
    if (type == TaskType::Interval)
    {
        equal &= (intervalSec == config.intervalSec);
    }
    else if (type == TaskType::TimePoint)
    {
        equal &= Commontool::isTimeListsContentEqual(timePoints, config.timePoints);
    }
    return equal;
}

QJsonObject TaskConfig::toJson() const
{
    QJsonObject obj;
    obj["taskName"] = taskName;
    obj["type"]     = int(type);  // 嵌套存储TaskType
    // 序列化时间点列表
    QJsonArray timeArray;
    for (const auto &time : timePoints)
    {
        timeArray.append(timeToJson(time));
    }
    obj["timePoints"]  = timeArray;
    obj["intervalSec"] = intervalSec;
    // 注意：callback不序列化，仅通过taskName后续映射恢复
    return obj;
}

TaskConfig TaskConfig::fromJson(const QJsonObject &obj)
{
    TaskConfig config;
    config.taskName = obj["taskName"].toString();
    config.type     = TaskType(obj["type"].toInt());
    // 反序列化时间点列表
    QJsonArray timeArray = obj["timePoints"].toArray();
    for (const auto &timeVal : timeArray)
    {
        config.timePoints.append(jsonToTime(timeVal.toObject()));
    }
    config.intervalSec = obj["intervalSec"].toInt();
    // callback初始化为空，需外部通过taskName设置（如：config.callback = callbackMap[config.taskName]）
    return config;
}
