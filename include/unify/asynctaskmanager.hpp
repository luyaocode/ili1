#ifndef TASKMANAGER_HPP
#define TASKMANAGER_HPP

#include "./asynctaskrunner.hpp"
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <thread>

NAMESPACE_BEGIN(unify)

#define MAX_CONCURRENT_SIZE 10  // 最大异步任务数量
// 任务ID类型
using TaskID = uint64_t;

// 定义任务基类接口
class ITask
{
public:
    virtual ~ITask()                        = default;
    virtual void          start()           = 0;
    virtual void          cancel()          = 0;
    virtual EM_TASK_STATE get_state() const = 0;
    virtual std::string   get_error() const = 0;
};

// 任务包装类
template<typename ResultType = void, typename ProgressData = float>
class TaskWrapper : public ITask
{
public:
    using TaskCallback     = typename AsyncTaskRunner<ResultType, ProgressData>::TaskCallback;
    using ProgressCallback = typename AsyncTaskRunner<ResultType, ProgressData>::ProgressCallback;
    using CompleteCallback = typename AsyncTaskRunner<ResultType, ProgressData>::CompleteCallback;
    using StartCallback    = typename AsyncTaskRunner<ResultType, ProgressData>::StartCallback;

    TaskWrapper(TaskCallback     task_cb,
                StartCallback    start_cb    = {},
                ProgressCallback progress_cb = {},
                CompleteCallback complete_cb = {})
        : m_task(std::move(task_cb), std::move(start_cb), std::move(progress_cb), std::move(complete_cb))
    {
    }

    void start() override
    {
        m_task.start();
    }
    void cancel() override
    {
        m_task.cancel();
    }
    EM_TASK_STATE get_state() const override
    {
        return m_task.get_state();
    }
    std::string get_error() const override
    {
        return m_task.get_error();
    }

    AsyncTaskRunner<ResultType, ProgressData> &get_task()
    {
        return m_task;
    }
    const AsyncTaskRunner<ResultType, ProgressData> &get_task() const
    {
        return m_task;
    }

private:
    AsyncTaskRunner<ResultType, ProgressData> m_task;
};

class AsyncTaskManager
{
public:
    // 获取单例实例
    static AsyncTaskManager &get_instance()
    {
        static AsyncTaskManager instance;
        return instance;
    }

    // 禁用拷贝
    AsyncTaskManager(const AsyncTaskManager &)            = delete;
    AsyncTaskManager &operator=(const AsyncTaskManager &) = delete;
    AsyncTaskManager(AsyncTaskManager &&)                 = delete;
    AsyncTaskManager &operator=(AsyncTaskManager &&)      = delete;

    // 重载1：非 void 类型
    template<typename ResultType, typename ProgressData = float>
    typename std::enable_if<!std::is_void<ResultType>::value, TaskID>::type
        add_task(typename AsyncTaskRunner<ResultType, ProgressData>::TaskCallback     task_cb,
                 typename AsyncTaskRunner<ResultType, ProgressData>::StartCallback    start_cb    = {},
                 typename AsyncTaskRunner<ResultType, ProgressData>::ProgressCallback progress_cb = {},
                 typename AsyncTaskRunner<ResultType, ProgressData>::CompleteCallback complete_cb = {})
    {
        auto task_id             = generate_task_id();
        auto wrapped_complete_cb = [this, task_id, complete_cb](EM_TASK_STATE state, ResultType result) {
            if (complete_cb)
                complete_cb(state, std::move(result));
            this->on_task_completed(task_id);
        };
        add_task_impl<ResultType, ProgressData>(task_id, std::move(task_cb), std::move(start_cb),
                                                std::move(progress_cb), std::move(wrapped_complete_cb));
        return task_id;
    }

    // 重载2：void 类型
    template<typename ResultType, typename ProgressData = float>
    typename std::enable_if<std::is_void<ResultType>::value, TaskID>::type
        add_task(typename AsyncTaskRunner<void, ProgressData>::TaskCallback     task_cb,
                 typename AsyncTaskRunner<void, ProgressData>::StartCallback    start_cb    = {},
                 typename AsyncTaskRunner<void, ProgressData>::ProgressCallback progress_cb = {},
                 typename AsyncTaskRunner<void, ProgressData>::CompleteCallback complete_cb = {})
    {
        auto task_id             = generate_task_id();
        auto wrapped_complete_cb = [this, task_id, complete_cb](EM_TASK_STATE state) {
            if (complete_cb)
                complete_cb(state);
            this->on_task_completed(task_id);
        };
        add_task_impl<void, ProgressData>(task_id, std::move(task_cb), std::move(start_cb), std::move(progress_cb),
                                          std::move(wrapped_complete_cb));
        return task_id;
    }

    // 取消任务
    bool cancel_task(TaskID task_id)
    {
        std::unique_lock<std::mutex> lock(m_task_mutex);

        auto it = m_task_map.find(task_id);
        if (it == m_task_map.end())
            return false;

        bool was_running = false;
        // 如果任务在队列中，直接移除
        if (is_task_queued(task_id))
        {
            remove_from_queue(task_id);
            m_task_map.erase(it);
        }
        else
        {
            // 否则取消执行中的任务
            auto &task = it->second;
            task->cancel();  // 现在可以正确调用cancel()

            // 任务取消后清理
            was_running = m_running_tasks.erase(task_id) > 0;
            m_task_map.erase(it);
        }

        // 解锁后再尝试启动新任务
        lock.unlock();

        if (was_running)
        {
            try_start_next_task();
        }

        return true;
    }

    // 获取任务状态
    EM_TASK_STATE get_task_state(TaskID task_id) const
    {
        std::lock_guard<std::mutex> lock(m_task_mutex);

        auto it = m_task_map.find(task_id);
        if (it == m_task_map.end())
            return EM_TASK_STATE::FAILED;  // 任务不存在

        return it->second->get_state();
    }

    // 获取任务结果（需指定类型）
    template<typename ResultType = void, typename ProgressData = float>
    ResultType get_task_result(TaskID task_id)
    {
        std::unique_lock<std::mutex> lock(m_task_mutex);

        auto it = m_task_map.find(task_id);
        if (it == m_task_map.end())
            throw std::runtime_error("Task not found");

        auto wrapper_ptr = std::dynamic_pointer_cast<TaskWrapper<ResultType, ProgressData>>(it->second);
        if (!wrapper_ptr)
            throw std::runtime_error("Task type mismatch");

        // 保存任务引用并解锁
        auto &task_ref = wrapper_ptr->get_task();
        lock.unlock();

        // 解锁后获取结果，避免长时间持有锁
        return task_ref.get_result();
    }

    // 设置最大并发数
    void set_max_concurrent_tasks(size_t max)
    {
        std::unique_lock<std::mutex> lock(m_task_mutex);
        m_max_concurrent = max;

        // 解锁后尝试启动任务
        lock.unlock();
        try_start_next_task();
    }

    // 获取当前并发任务数
    size_t get_running_task_count() const
    {
        std::lock_guard<std::mutex> lock(m_task_mutex);
        return m_running_tasks.size();
    }

    // 清理已完成的任务
    void cleanup_completed_tasks()
    {
        std::vector<TaskID> to_remove;

        {
            std::lock_guard<std::mutex> lock(m_task_mutex);

            // 收集已完成的任务ID
            for (const auto &pair : m_task_map)
            {
                TaskID task_id = pair.first;
                auto  &task    = pair.second;

                EM_TASK_STATE state = task->get_state();
                if (state == EM_TASK_STATE::COMPLETED || state == EM_TASK_STATE::CANCELLED ||
                    state == EM_TASK_STATE::FAILED)
                {
                    to_remove.push_back(task_id);
                }
            }

            // 移除已完成的任务
            for (TaskID task_id : to_remove)
            {
                m_task_map.erase(task_id);
                m_running_tasks.erase(task_id);
            }
        }

        // 清理完成后尝试启动新任务
        if (!to_remove.empty())
        {
            try_start_next_task();
        }
    }

private:
    AsyncTaskManager(): m_next_task_id(1), m_max_concurrent(MAX_CONCURRENT_SIZE)
    {
    }

    ~AsyncTaskManager()
    {
        // 清理所有任务
        std::lock_guard<std::mutex> lock(m_task_mutex);
        m_task_map.clear();

        while (!m_task_queue.empty())
        {
            m_task_queue.pop();
        }
        m_running_tasks.clear();
    }

    // 私有辅助函数：提取公共逻辑（创建任务、加锁解锁、入队等）
    template<typename ResultType, typename ProgressData>
    void add_task_impl(TaskID                                                               task_id,
                       typename AsyncTaskRunner<ResultType, ProgressData>::TaskCallback     task_cb,
                       typename AsyncTaskRunner<ResultType, ProgressData>::StartCallback    start_cb,
                       typename AsyncTaskRunner<ResultType, ProgressData>::ProgressCallback progress_cb,
                       typename AsyncTaskRunner<ResultType, ProgressData>::CompleteCallback complete_cb)
    {
        auto task = std::make_shared<TaskWrapper<ResultType, ProgressData>>(
            std::move(task_cb), std::move(start_cb), std::move(progress_cb), std::move(complete_cb));

        std::unique_lock<std::mutex> lock(m_task_mutex);
        m_task_map[task_id] = task;
        m_task_queue.push(task_id);
        lock.unlock();

        try_start_next_task();
    }

    // 生成唯一任务ID
    TaskID generate_task_id()
    {
        return m_next_task_id++;
    }

    // 检查任务是否在队列中
    bool is_task_queued(TaskID task_id) const
    {
        // 注意：调用此方法时必须持有锁
        std::queue<TaskID> temp_queue = m_task_queue;
        while (!temp_queue.empty())
        {
            if (temp_queue.front() == task_id)
                return true;
            temp_queue.pop();
        }
        return false;
    }

    // 从队列中移除任务
    void remove_from_queue(TaskID task_id)
    {
        // 注意：调用此方法时必须持有锁
        std::queue<TaskID> new_queue;
        while (!m_task_queue.empty())
        {
            if (m_task_queue.front() != task_id)
                new_queue.push(m_task_queue.front());
            m_task_queue.pop();
        }
        m_task_queue = std::move(new_queue);
    }

    // 任务完成回调处理（异步执行）
    void on_task_completed(TaskID task_id)
    {
        // 在新线程中处理，避免回调死锁
        std::thread([this, task_id]() {
            // 清理完成的任务
            this->cleanup_completed_tasks();

            // 尝试启动下一个任务
            this->try_start_next_task();
        }).detach();
    }

    // 尝试启动队列中的下一个任务
    void try_start_next_task()
    {
        std::unique_lock<std::mutex> lock(m_task_mutex);

        // 检查是否可以启动新任务
        while (m_running_tasks.size() < m_max_concurrent && !m_task_queue.empty())
        {
            TaskID task_id = m_task_queue.front();
            m_task_queue.pop();

            auto it = m_task_map.find(task_id);
            if (it == m_task_map.end())
                continue;

            auto task = it->second;

            // 标记为运行中
            m_running_tasks.insert(task_id);

            // 解锁后设置回调和启动任务，避免死锁
            lock.unlock();

            // 启动任务（利用AsyncTaskRunner的异步特性）
            task->start();

            // 重新加锁继续处理队列
            lock.lock();
            // TODO
        }

        // unique_lock 会自动解锁
    }

    // 任务ID生成器
    std::atomic<TaskID> m_next_task_id;

    // 任务存储（使用ITask接口）
    mutable std::mutex                                 m_task_mutex;
    std::unordered_map<TaskID, std::shared_ptr<ITask>> m_task_map;
    std::queue<TaskID>                                 m_task_queue;
    std::unordered_set<TaskID>                         m_running_tasks;

    // 并发控制
    size_t m_max_concurrent;
};

NAMESPACE_END

#endif  // TASKMANAGER_HPP
