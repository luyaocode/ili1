#ifndef MULTIMultiScheduledTaskMgr_H
#define MULTIMultiScheduledTaskMgr_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QTime>
#include <QList>
#include <QHash>
#include <QJsonObject>
#include <QDebug>
#include <functional>
#include <mutex>
#include <memory>

#define MAX_TASK_SIZE 100  // 最大计划数量
// 任务ID类型
using TaskID = uint64_t;

// 任务类型：时间点触发 / 间隔触发
enum class TaskType
{
    TimePoint,  // 特定时间点触发（如每天8:30、12:00）
    Interval    // 间隔触发（如每30分钟、每2小时）
};

class MultiScheduledTaskMgr;
// 单个任务的配置（支持任意参数的回调）
struct TaskConfig
{
    QString               taskName;         // 任务名称
    TaskType              type;             // 任务类型
    QList<QTime>          timePoints;       // 时间点列表（仅TimePoint有效）
    int                   intervalSec = 0;  // 间隔秒数（仅Interval有效）
    std::function<void()> callback;         // 包装后的回调（可含任意参数）
    bool                  operator==(const TaskConfig &config);
    QJsonObject           toJson() const;
    static TaskConfig     fromJson(const QJsonObject &obj);

private:
    friend class MultiScheduledTaskMgr;
    TaskID id;  // 任务ID(唯一)
};

// 定时任务管理器（线程安全的单例模式）
class MultiScheduledTaskMgr : public QObject
{
    Q_OBJECT
public:
    // 禁用拷贝构造和赋值运算（单例禁止复制）
    MultiScheduledTaskMgr(const MultiScheduledTaskMgr &)            = delete;
    MultiScheduledTaskMgr &operator=(const MultiScheduledTaskMgr &) = delete;

    // 获取单例实例（线程安全）
    static MultiScheduledTaskMgr &getInstance();

    TaskID addTask(TaskConfig &config);
    bool   removeTask(TaskID id);
    bool   updateTask(const TaskConfig &config);

    // 添加任务（支持任意参数的回调绑定，线程安全）
    template<typename Func, typename... Args>
    TaskID addTask(const QString &taskName, Func &&func, Args &&...args, const QList<QTime> &timePoints)
    {
        TaskConfig config;
        config.taskName    = taskName;
        config.type        = TaskType::TimePoint;
        config.timePoints  = timePoints;
        config.intervalSec = 0;
        config.callback    = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
        return addTask(config);
    }

    // 添加任务（支持任意参数的回调绑定，线程安全）
    template<typename Func, typename... Args>
    TaskID addTask(const QString &taskName, Func &&func, Args &&...args, int intervalSec = 0)
    {
        TaskConfig config;
        config.taskName    = taskName;
        config.type        = TaskType::Interval;
        config.timePoints  = {};
        config.intervalSec = intervalSec;
        config.callback    = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
        return addTask(config);
    }

    // ==========支持成员函数的时间点任务模板 ==========
    // 模板参数：Class - 成员函数所属的类类型；Func - 成员函数指针类型；Args - 成员函数的参数类型包
    template<typename Class, typename Func, typename... Args>
    TaskID addTimePointTask(const QString &taskName,
                            Func         &&memberFunc,  // 成员函数指针（如 &MyClass::func）
                            Class *instance,  // 类实例指针（this 或 new 出来的对象，需保证生命周期）
                            Args &&...args,  // 成员函数的参数（任意数量/类型）
                            const QList<QTime> &timePoints)
    {  // 固定时间点参数（在参数包后）
        TaskConfig config;
        config.taskName    = taskName;
        config.type        = TaskType::TimePoint;
        config.timePoints  = timePoints;
        config.intervalSec = 0;
        config.callback    = std::bind(std::forward<Func>(memberFunc),  // 成员函数指针
                                       instance,  // 绑定实例（this指针，成员函数的隐含第一个参数）
                                       std::forward<Args>(args)...  // 展开成员函数的参数包
           );
        return addTask(config);
    }

    // ========== 核心：支持成员函数的间隔任务模板 ==========
    // 模板参数：Class-成员函数所属类；Func-成员函数指针；Args-成员函数参数类型包
    template<typename Class, typename Func, typename... Args>
    TaskID addTask(const QString &taskName,
                   Func         &&memberFunc,  // 成员函数指针（如 &MyClass::func）
                   Class         *instance,    // 类实例指针（this/对象指针，需保证生命周期）
                   Args &&...args,             // 成员函数的任意参数（可变长度/类型）
                   int intervalSec = 0)
    {
        TaskConfig config;
        config.taskName    = taskName;
        config.type        = TaskType::Interval;
        config.timePoints  = {};
        config.intervalSec = intervalSec;
        config.callback = std::bind(std::forward<Func>(memberFunc),  // 转发成员函数指针（保持类型属性）
                                    instance,  // 绑定实例（成员函数必须依赖实例调用）
                                    std::forward<Args>(args)...  // 展开并转发成员函数的参数包
        );
        return addTask(config);
    }

    // ========== 间隔任务：只接收绑定后的 std::function<void()> ==========
    TaskID addTask(const QString        &taskName,
                   std::function<void()> callback,  // 已绑定所有参数的回调
                   int                   intervalSec = 0);

    // ========== 时间点任务：只接收绑定后的函数 ==========
    TaskID addTask(const QString &taskName, std::function<void()> callback, const QList<QTime> &timePoints);
private:
    // 私有构造函数（禁止外部创建）
    explicit MultiScheduledTaskMgr(QObject *parent = nullptr);
    // 校验任务配置合法性
    bool validateTask(const TaskConfig &config);
    // 检查所有任务是否需要执行（定时器触发，线程安全）
    void checkTasks();
    // 检查时间点任务
    void checkTimePointTask(const TaskConfig &task, const QDateTime &now);
    // 检查间隔任务
    void checkIntervalTask(const TaskConfig &task, const QDateTime &now);
    // 执行任务
    void runTask(const TaskConfig &task);
    // 生成唯一任务ID
    TaskID generate_task_id();

private:
    // 单例实例（静态成员）
    static MultiScheduledTaskMgr *m_instance;
    static std::mutex             m_mutex;  // 保护单例创建的互斥锁

    QTimer                   *m_checkTimer;    // 任务检查定时器
    QList<TaskConfig>         m_tasks;         // 任务列表
    std::mutex                m_taskMutex;     // 保护任务列表读写的互斥锁
    QHash<TaskID, QDateTime> m_lastRun;       // 记录任务上次执行时间
    std::mutex                m_lastRunMutex;  // 保护上次执行任务时间的互斥锁
    // 任务ID生成器
    std::atomic<TaskID> m_next_task_id;
    size_t              m_max_task_size;
};

#endif  // MULTIMultiScheduledTaskMgr_H
