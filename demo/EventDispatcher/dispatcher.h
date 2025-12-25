#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <QThread>
#include <QThreadPool>
#include <QRunnable>
#include <QElapsedTimer>
#include <QDebug>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <utility>

// --------------------------
// 定义Promise基类
// --------------------------
class PromiseBase
{
public:
    virtual ~PromiseBase() = default;
    // 虚函数：设置异常（所有类型通用）
    virtual void setException(std::exception_ptr e) = 0;
};

// 模板子类：存储具体类型的Promise
template<typename RetType>
class PromiseWrapper : public PromiseBase
{
public:
    explicit PromiseWrapper(std::shared_ptr<std::promise<RetType>> promise): m_promise(promise)
    {
    }

    // 设置返回值
    void setValue(RetType &&value)
    {
        m_promise->set_value(std::move(value));
    }

    // 实现基类虚函数：设置异常
    void setException(std::exception_ptr e) override
    {
        m_promise->set_exception(e);
    }

    std::shared_ptr<std::promise<RetType>> getPromise() const
    {
        return m_promise;
    }

private:
    std::shared_ptr<std::promise<RetType>> m_promise;
};

// 无返回值特化版本
template<>
class PromiseWrapper<void> : public PromiseBase
{
public:
    explicit PromiseWrapper(std::shared_ptr<std::promise<void>> promise): m_promise(promise)
    {
    }

    // 无返回值：标记完成
    void setValue()
    {
        m_promise->set_value();
    }

    void setException(std::exception_ptr e) override
    {
        m_promise->set_exception(e);
    }

    std::shared_ptr<std::promise<void>> getPromise() const
    {
        return m_promise;
    }

private:
    std::shared_ptr<std::promise<void>> m_promise;
};

// --------------------------
// 通用DBus任务类
// --------------------------
class GenericDbusTask : public QRunnable
{
public:
    // 有返回值的任务构造函数
    template<typename RetType>
    GenericDbusTask(std::shared_ptr<PromiseWrapper<RetType>> promiseWrapper,
                    const std::function<RetType()>          &taskFunc,
                    int                                      timeoutMs)
        : m_promiseBase(promiseWrapper), m_timeoutMs(timeoutMs)
    {
        // 封装任务逻辑（C++11 lambda兼容）
        m_task = [promiseWrapper, taskFunc, timeoutMs]() {
            try
            {
                QElapsedTimer timer;
                timer.start();

                // 执行DBus调用
                RetType result = taskFunc();

                // 超时检查
                if (timer.elapsed() > timeoutMs)
                {
                    throw std::runtime_error("DBus call timeout");
                }

                // 设置返回值
                promiseWrapper->setValue(std::move(result));
            }
            catch (const std::exception &e)
            {
                // 传递异常
                promiseWrapper->setException(std::current_exception());
            }
            catch (...)
            {
                promiseWrapper->setException(std::make_exception_ptr(std::runtime_error("Unknown DBus error")));
            }
        };
        setAutoDelete(true);
    }

    // 无返回值的任务构造函数
    GenericDbusTask(std::shared_ptr<PromiseWrapper<void>> promiseWrapper,
                    const std::function<void()>          &taskFunc,
                    int                                   timeoutMs)
        : m_promiseBase(promiseWrapper), m_timeoutMs(timeoutMs)
    {
        m_task = [promiseWrapper, taskFunc, timeoutMs]() {
            try
            {
                QElapsedTimer timer;
                timer.start();

                taskFunc();

                if (timer.elapsed() > timeoutMs)
                {
                    throw std::runtime_error("DBus call timeout");
                }

                // 标记任务完成
                promiseWrapper->setValue();
            }
            catch (const std::exception &e)
            {
                promiseWrapper->setException(std::current_exception());
            }
            catch (...)
            {
                promiseWrapper->setException(std::make_exception_ptr(std::runtime_error("Unknown DBus error")));
            }
        };
        setAutoDelete(true);
    }

    void run() override
    {
        qDebug() << "[GenericDbusTask]" << QThread::currentThreadId() << "开始执行DBus任务";
        m_task();
        qDebug() << "[GenericDbusTask]" << QThread::currentThreadId() << "DBus任务执行完成";
    }

private:
    std::shared_ptr<PromiseBase> m_promiseBase;  // 基类指针存储任意类型Promise
    std::function<void()>        m_task;         // 任务逻辑
    int                          m_timeoutMs;    // 超时时间
};

// --------------------------
// 事件分发器线程
// --------------------------
class DispatcherThread : public QThread
{
    Q_OBJECT
public:
    explicit DispatcherThread(QObject *parent = nullptr);

    ~DispatcherThread() override;

    void sendDbusMessage(const char         *interfaceName,
                         const QString      &strName,
                         const QVariantList &args,
                         const QString      &strPath);

    template<typename RetType, typename... Args>
    std::future<RetType>
        callDbusInterface(std::function<RetType(Args...)> dbusFunc, Args &&...args, int timeoutMs = 3000)
    {
        // 创建Promise和Future
        auto                 promise = std::make_shared<std::promise<RetType>>();
        std::future<RetType> future  = promise->get_future();

        // 包装Promise
        auto promiseWrapper = std::make_shared<PromiseWrapper<RetType>>(promise);

        // 绑定参数并转为std::function
        std::function<RetType()> taskFunc = std::bind(dbusFunc, std::forward<Args>(args)...);

        // 创建任务并提交
        GenericDbusTask *task = new GenericDbusTask(promiseWrapper, taskFunc, timeoutMs);
        m_pool.start(static_cast<QRunnable *>(task));

        qDebug() << "[DispatcherThread]" << QThread::currentThreadId() << "提交通用DBus任务到线程池";

        return future;
    }

    // 通用DBus调用接口（无返回值）
    template<typename... Args>
    std::future<void> callDbusInterface(std::function<void(Args...)> dbusFunc, Args &&...args, int timeoutMs = 3000)
    {
        auto              promise = std::make_shared<std::promise<void>>();
        std::future<void> future  = promise->get_future();

        auto                  promiseWrapper = std::make_shared<PromiseWrapper<void>>(promise);
        std::function<void()> taskFunc       = std::bind(dbusFunc, std::forward<Args>(args)...);

        GenericDbusTask *task = new GenericDbusTask(promiseWrapper, taskFunc, timeoutMs);
        m_pool.start(static_cast<QRunnable *>(task));

        qDebug() << "[DispatcherThread]" << QThread::currentThreadId() << "提交无返回值DBus任务到线程池（无阻塞）";

        return future;
    }

protected:
    void run() override;

private:
    QThreadPool m_pool;
};

#endif  // DISPATCHER_H
