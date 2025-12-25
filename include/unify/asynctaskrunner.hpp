#ifndef ASYNCTASKRUNNER_HPP
#define ASYNCTASKRUNNER_HPP
#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <unordered_map>
#include "./global_def.h"

NAMESPACE_BEGIN(unify)
inline void set_thread_name(const std::string &name)
{
    // 注意：Linux线程名称长度限制为15个字符（不含终止符）
    std::string truncated_name = name.substr(0, 15);
    pthread_setname_np(pthread_self(), truncated_name.c_str());
}

// 任务进度
template<typename ProgressData = float>
struct TaskProgress
{
    float        percentage;   // 进度
    ProgressData custom_data;  // 用户数据
    std::string  message;      // 执行过程信息

    TaskProgress(float pct = 0.0f, const std::string &msg = "")
        : percentage(std::min(100.0f, std::max(0.0f, pct))), custom_data(percentage), message(msg)
    {
    }

    TaskProgress(float pct, ProgressData data, const std::string &msg = "")
        : percentage(std::min(100.0f, std::max(0.0f, pct))), custom_data(std::move(data)), message(msg)
    {
    }
};

// 任务状态
enum class EM_TASK_STATE
{
    PENDING,    // 空闲
    RUNNING,    // 执行中
    COMPLETED,  // 已完成
    CANCELLED,  // 被取消
    FAILED      // 失败
};

// 使用模板特化定义回调类型
template<typename ResultType>
struct CompleteCallbackType;

// 非void返回类型的回调
template<typename ResultType>
struct CompleteCallbackType
{
    using type = std::function<void(EM_TASK_STATE, ResultType)>;
};

// void返回类型的回调（特化版本）
template<>
struct CompleteCallbackType<void>
{
    using type = std::function<void(EM_TASK_STATE)>;
};

template<typename ResultType = void, typename ProgressData = float>
class AsyncTaskRunner
{
public:
    using UpdateCallback     = std::function<void(float, ProgressData, std::string)>;
    using IsCanceledCallback = std::function<bool()>;

    using TaskCallback     = std::function<ResultType(const UpdateCallback &, const IsCanceledCallback &)>;
    using ProgressCallback = std::function<void(const TaskProgress<ProgressData> &)>;

    using StartCallback = std::function<void()>;
    // 使用特化模板定义回调类型
    using CompleteCallback = typename CompleteCallbackType<ResultType>::type;

public:
    explicit AsyncTaskRunner(TaskCallback     task_cb,
                             StartCallback    start_cb    = {},
                             ProgressCallback progress_cb = {},
                             CompleteCallback complete_cb = {})
        : m_task_cb(std::move(task_cb))
        , m_start_cb(std::move(start_cb))
        , m_progress_cb(std::move(progress_cb))
        , m_complete_cb(std::move(complete_cb))
        , m_state(EM_TASK_STATE::PENDING)
        , m_is_cancelled(false)
    {
    }

    AsyncTaskRunner(const AsyncTaskRunner &)            = delete;
    AsyncTaskRunner &operator=(const AsyncTaskRunner &) = delete;
    AsyncTaskRunner(AsyncTaskRunner &&)                 = delete;
    AsyncTaskRunner &operator=(AsyncTaskRunner &&)      = delete;

    ~AsyncTaskRunner()
    {
        cancel();
        if (m_future.valid())
        {
            m_future.wait();
        }
    }

    // 执行任务
    void start()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_state != EM_TASK_STATE::PENDING)
        {
            throw std::runtime_error("Task has already been started");
        }

        m_is_cancelled = false;
        m_state        = EM_TASK_STATE::RUNNING;

        // pct 当前进度
        // data 自定义数据
        // msg 消息
        auto update_progress = [this](float pct, ProgressData data, std::string msg) {
            if (m_is_cancelled)
                return;
            TaskProgress<ProgressData>  progress(pct, std::move(data), std::move(msg));
            std::lock_guard<std::mutex> lock(m_cb_mutex);
            if (m_progress_cb)
            {
                m_progress_cb(progress);
            }
        };

        auto is_task_cancelled = [this]() -> bool {
            return m_is_cancelled;
        };
        // 调用模板辅助函数执行任务
        m_future = execute_task(update_progress, is_task_cancelled);
    }

    void cancel()
    {
        m_is_cancelled = true;
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_state == EM_TASK_STATE::RUNNING)
        {
            m_state = EM_TASK_STATE::CANCELLED;
        }
    }

    EM_TASK_STATE get_state() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state;
    }

    ResultType get_result()
    {
        if (get_state() == EM_TASK_STATE::PENDING)
        {
            throw std::runtime_error("Task not started");
        }
        if (get_state() == EM_TASK_STATE::CANCELLED)
        {
            throw std::runtime_error("Task was cancelled");
        }
        return m_future.get();
    }

    std::string get_error() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_error_msg;
    }

    void set_progress_callback(ProgressCallback cb)
    {
        std::lock_guard<std::mutex> lock(m_cb_mutex);
        m_progress_cb = std::move(cb);
    }

    void set_complete_callback(CompleteCallback cb)
    {
        std::lock_guard<std::mutex> lock(m_cb_mutex);
        m_complete_cb = std::move(cb);
    }

private:
    // 任务失败设置状态
    void on_start()
    {
        std::lock_guard<std::mutex> lock(m_cb_mutex);
        if (m_start_cb)
        {
            m_start_cb();
        }
    }
    // 任务结束设置状态
    // 辅助函数：调用回调（void版本）
    template<typename T = ResultType>
    typename std::enable_if<std::is_void<T>::value>::type on_completed()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_state != EM_TASK_STATE::CANCELLED)
            {
                m_state = EM_TASK_STATE::COMPLETED;
            }
        }
        std::unique_lock<std::mutex> lock(m_cb_mutex);
        // 保存回调副本并解锁，避免死锁
        CompleteCallback cb_copy = m_complete_cb;
        lock.unlock();

        // 调用回调（void版本）
        if (cb_copy)
        {
            cb_copy(m_state);
        }
    }

    // 辅助函数：调用回调（非void版本）
    template<typename T = ResultType>
    typename std::enable_if<!std::is_void<T>::value>::type on_completed(T result)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_state != EM_TASK_STATE::CANCELLED)
            {
                m_state = EM_TASK_STATE::COMPLETED;
            }
        }
        std::unique_lock<std::mutex> lock(m_cb_mutex);
        // 保存回调副本并解锁，避免死锁
        CompleteCallback cb_copy = m_complete_cb;
        lock.unlock();

        // 调用回调（带结果）
        if (cb_copy)
        {
            cb_copy(m_state, std::move(result));
        }
    }
    // 任务失败设置状态
    void on_failed()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_state = EM_TASK_STATE::FAILED;
    }
    // 辅助函数：执行非void返回值任务
    template<typename T = ResultType>
    typename std::enable_if<!std::is_void<T>::value, std::future<T>>::type
        execute_task(const UpdateCallback &update_progress, const IsCanceledCallback &is_task_cancelled)
    {
        return std::async(std::launch::async, [this, update_progress, is_task_cancelled]() -> T {
            set_thread_name("t_async_task");
            try
            {
                on_start();
                T result = m_task_cb(update_progress, is_task_cancelled);
                on_completed(result);
                return result;
            }
            catch (const std::exception &e)
            {
                on_failed();
                m_error_msg = e.what();
                throw;
            }
            catch (...)
            {
                on_failed();
                m_error_msg = "Unknown error";
                throw;
            }
        });
    }

    // 辅助函数：执行void返回值任务
    template<typename T = ResultType>
    typename std::enable_if<std::is_void<T>::value, std::future<void>>::type
        execute_task(const UpdateCallback &update_progress, const IsCanceledCallback &is_task_cancelled)
    {
        return std::async(std::launch::async, [this, update_progress, is_task_cancelled]() {
            set_thread_name("t_async_task");
            try
            {
                on_start();
                m_task_cb(update_progress, is_task_cancelled);  // 无返回值
                on_completed();
            }
            catch (const std::exception &e)
            {
                on_failed();
                m_error_msg = e.what();
                throw;
            }
            catch (...)
            {
                on_failed();
                m_error_msg = "Unknown error";
                throw;
            }
        });
    }

private:
    TaskCallback            m_task_cb;       // 回调函数
    StartCallback           m_start_cb;      // 启动时回调函数
    ProgressCallback        m_progress_cb;   // 进行时回调函数
    CompleteCallback        m_complete_cb;   // 完成时回调函数
    std::future<ResultType> m_future;        // 执行结果
    mutable std::mutex      m_mutex;         // 状态成员m_state互斥锁
    mutable std::mutex      m_cb_mutex;      // 回调函数m_xxx_cb互斥锁
    EM_TASK_STATE           m_state;         // 任务状态
    std::atomic<bool>       m_is_cancelled;  // 是否取消
    std::string             m_error_msg;
};
template<typename ResultType = void>
using SimpleAsyncTask = AsyncTaskRunner<ResultType, float>;

NAMESPACE_END

#endif
