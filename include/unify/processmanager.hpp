#ifndef PROCESS_MANAGER_HPP
#define PROCESS_MANAGER_HPP

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include <cstring>
#include <cerrno>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <stdexcept>
#include <functional>
#include <chrono>
#include <memory>
#include <atomic>
#include <algorithm>
#include "./global_def.h"

NAMESPACE_BEGIN(unify)

// 前向声明
class Process;
class ProcessManager;

/**
 * @brief 进程异常基类
 */
class ProcessException : public std::runtime_error
{
public:
    explicit ProcessException(const std::string &what_arg): std::runtime_error(what_arg)
    {
    }
    explicit ProcessException(const char *what_arg): std::runtime_error(what_arg)
    {
    }
};

/**
 * @brief 进程创建失败异常
 */
class ProcessCreationFailed : public ProcessException
{
public:
    explicit ProcessCreationFailed(const std::string &what_arg): ProcessException(what_arg)
    {
    }
};

/**
 * @brief 进程不存在异常
 */
class ProcessNotFound : public ProcessException
{
public:
    explicit ProcessNotFound(pid_t pid): ProcessException("Process with PID " + std::to_string(pid) + " not found")
    {
    }
};

/**
 * @brief 权限不足异常
 */
class ProcessPermissionDenied : public ProcessException
{
public:
    explicit ProcessPermissionDenied(const std::string &what_arg): ProcessException(what_arg)
    {
    }
};

/**
 * @brief 进程状态枚举
 */
enum class ProcessState
{
    NotStarted,  // 未启动
    Running,     // 运行中
    Exited,      // 正常退出
    Killed,      // 被杀死
    Crashed      // 异常崩溃
};

/**
 * @brief 文件重定向配置
 */
struct RedirectConfig
{
    std::string stdin_path;     // 标准输入重定向路径
    std::string stdout_path;    // 标准输出重定向路径
    std::string stderr_path;    // 标准错误重定向路径
    bool        append = true;  // 是否追加模式
};

/**
 * @brief 单个进程封装类
 */
class Process
{
    friend class ProcessManager;

public:
    /**
     * @brief 构造函数
     * @param cmd 命令行参数列表（第一个元素为程序路径）
     * @param working_dir 工作目录
     * @param redirect 重定向配置
     */
    Process(std::vector<std::string> cmd, std::string working_dir = "", RedirectConfig redirect = RedirectConfig {})
        : cmd_(std::move(cmd))
        , working_dir_(std::move(working_dir))
        , redirect_(std::move(redirect))
        , pid_(-1)
        , exit_code_(0)
        , state_(ProcessState::NotStarted)
        , start_time_(0)
        , end_time_(0)
    {
    }

    // 禁止拷贝（进程句柄唯一）
    Process(const Process &)            = delete;
    Process &operator=(const Process &) = delete;

    // 允许移动
    Process(Process &&other) noexcept
    {
        std::lock_guard<std::mutex> lock(other.mutex_);
        swap(*this, other);
    }

    Process &operator=(Process &&other) noexcept
    {
        if (this != &other)
        {
            std::lock_guard<std::mutex> lock_this(mutex_);
            std::lock_guard<std::mutex> lock_other(other.mutex_);
            swap(*this, other);
        }
        return *this;
    }

    /**
     * @brief 启动进程
     * @throw ProcessCreationFailed 进程创建失败时抛出
     */
    void start()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != ProcessState::NotStarted)
        {
            throw ProcessException("Process already started or exited");
        }

        pid_t pid = fork();
        if (pid < 0)
        {
            throw ProcessCreationFailed("fork() failed: " + std::string(strerror(errno)));
        }

        // 子进程
        if (pid == 0)
        {
            try
            {
                // 设置工作目录
                if (!working_dir_.empty())
                {
                    if (chdir(working_dir_.c_str()) == -1)
                    {
                        _exit(EXIT_FAILURE);
                    }
                }

                // 处理文件重定向
                setup_redirect();

                // 转换命令参数
                std::vector<char *> argv;
                for (auto &arg : cmd_)
                {
                    argv.push_back(const_cast<char *>(arg.c_str()));
                }
                argv.push_back(nullptr);

                // 执行程序
                execvp(argv[0], argv.data());

                // execvp 失败
                _exit(EXIT_FAILURE);
            }
            catch (...)
            {
                _exit(EXIT_FAILURE);
            }
        }

        // 父进程
        pid_   = pid;
        state_ = ProcessState::Running;
        start_time_ =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
    }

    /**
     * @brief 等待进程结束（阻塞）
     * @return 进程退出码
     */
    int wait()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return wait_internal(true);
    }

    /**
     * @brief 非阻塞等待进程结束
     * @return 是否已结束
     */
    bool try_wait()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ != ProcessState::Running)
        {
            return true;
        }
        return wait_internal(false) >= 0;
    }

    /**
     * @brief 终止进程
     * @param sig 终止信号（默认 SIGTERM）
     * @return 是否成功
     */
    bool terminate(int sig = SIGTERM)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (state_ != ProcessState::Running)
        {
            return false;
        }

        if (::kill(pid_, sig) == -1)
        {
            if (errno == ESRCH)
            {
                update_state();
            }
            return false;
        }

        // 等待进程退出
        int status;
        if (waitpid(pid_, &status, 0) == pid_)
        {
            update_state_from_status(status);
            if (WIFSIGNALED(status))
            {
                state_ = ProcessState::Killed;
            }
        }

        return true;
    }

    /**
     * @brief 强制终止进程（SIGKILL）
     * @return 是否成功
     */
    bool kill()
    {
        return terminate(SIGKILL);
    }

    /**
     * @brief 获取进程ID
     * @return 进程ID（-1 表示未启动）
     */
    pid_t get_pid() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return pid_;
    }

    /**
     * @brief 获取进程状态
     * @return 进程状态
     */
    ProcessState get_state() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 检查状态是否过期
        if (state_ == ProcessState::Running)
        {
            const_cast<Process *>(this)->update_state();
        }
        return state_;
    }

    /**
     * @brief 获取退出码
     * @return 退出码（仅进程结束后有效）
     */
    int get_exit_code() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return exit_code_;
    }

    /**
     * @brief 获取启动时间（秒级时间戳）
     * @return 启动时间
     */
    uint64_t get_start_time() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return start_time_;
    }

    /**
     * @brief 获取结束时间（秒级时间戳）
     * @return 结束时间
     */
    uint64_t get_end_time() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return end_time_;
    }

    /**
     * @brief 获取运行时长（秒）
     * @return 运行时长
     */
    uint64_t get_running_time() const noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (state_ == ProcessState::Running)
        {
            auto now =
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count();
            return now - start_time_;
        }
        return end_time_ - start_time_;
    }

    /**
     * @brief 检查进程是否正在运行
     * @return 是否运行中
     */
    bool is_running() const
    {
        return get_state() == ProcessState::Running;
    }

private:
    /**
     * @brief 交换两个进程对象
     */
    static void swap(Process &a, Process &b) noexcept
    {
        using std::swap;
        swap(a.cmd_, b.cmd_);
        swap(a.working_dir_, b.working_dir_);
        swap(a.redirect_, b.redirect_);
        swap(a.pid_, b.pid_);
        swap(a.exit_code_, b.exit_code_);
        swap(a.state_, b.state_);
        swap(a.start_time_, b.start_time_);
        swap(a.end_time_, b.end_time_);
    }

    /**
     * @brief 设置文件重定向
     */
    void setup_redirect()
    {
        // 重定向标准输入
        if (!redirect_.stdin_path.empty())
        {
            int fd = open(redirect_.stdin_path.c_str(), O_RDONLY);
            if (fd >= 0)
            {
                dup2(fd, STDIN_FILENO);
                close(fd);
            }
        }

        // 重定向标准输出
        if (!redirect_.stdout_path.empty())
        {
            int flags = O_WRONLY | O_CREAT;
            flags |= redirect_.append ? O_APPEND : O_TRUNC;
            int fd = open(redirect_.stdout_path.c_str(), flags, 0644);
            if (fd >= 0)
            {
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }
        }

        // 重定向标准错误
        if (!redirect_.stderr_path.empty())
        {
            int flags = O_WRONLY | O_CREAT;
            flags |= redirect_.append ? O_APPEND : O_TRUNC;
            int fd = open(redirect_.stderr_path.c_str(), flags, 0644);
            if (fd >= 0)
            {
                dup2(fd, STDERR_FILENO);
                close(fd);
            }
        }
    }

    /**
     * @brief 内部等待实现
     * @param block 是否阻塞
     * @return 退出码（-1 表示未结束）
     */
    int wait_internal(bool block)
    {
        if (state_ != ProcessState::Running)
        {
            return exit_code_;
        }

        int   status;
        int   options = block ? 0 : WNOHANG;
        pid_t ret     = waitpid(pid_, &status, options);

        if (ret == -1)
        {
            if (errno == ECHILD)
            {
                state_    = ProcessState::Exited;
                end_time_ = std::chrono::duration_cast<std::chrono::seconds>(
                                std::chrono::system_clock::now().time_since_epoch())
                                .count();
                return exit_code_;
            }
            return -1;
        }

        if (ret == 0)
        {  // 非阻塞模式下进程仍在运行
            return -1;
        }

        // 进程已结束，更新状态
        update_state_from_status(status);
        end_time_ =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        return exit_code_;
    }

    /**
     * @brief 从 waitpid 状态更新进程状态
     */
    void update_state_from_status(int status)
    {
        if (WIFEXITED(status))
        {
            state_     = ProcessState::Exited;
            exit_code_ = WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            state_     = ProcessState::Killed;
            exit_code_ = WTERMSIG(status);
        }
        else if (WIFSTOPPED(status))
        {
            // 进程被暂停，暂不修改状态
        }
        else
        {
            state_     = ProcessState::Crashed;
            exit_code_ = -1;
        }
    }

    /**
     * @brief 更新进程状态
     */
    void update_state()
    {
        if (pid_ <= 0 || state_ != ProcessState::Running)
        {
            return;
        }

        int   status;
        pid_t ret = waitpid(pid_, &status, WNOHANG);

        if (ret == pid_)
        {
            update_state_from_status(status);
            end_time_ =
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count();
        }
        else if (ret == -1 && errno == ECHILD)
        {
            state_ = ProcessState::Exited;
            end_time_ =
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count();
        }
    }

    // 进程参数
    std::vector<std::string> cmd_;
    std::string              working_dir_;
    RedirectConfig           redirect_;

    // 进程状态
    pid_t        pid_;
    int          exit_code_;
    ProcessState state_;
    uint64_t     start_time_;
    uint64_t     end_time_;

    // 线程安全
    mutable std::mutex mutex_;
};

/**
 * @brief 多进程管理器
 */
class ProcessManager
{
public:
    using Ptr = std::shared_ptr<Process>;

    ProcessManager() = default;
    ~ProcessManager()
    {
        // 优雅关闭所有进程
        stop_all();
    }

    // 禁止拷贝
    ProcessManager(const ProcessManager &)            = delete;
    ProcessManager &operator=(const ProcessManager &) = delete;

    // 允许移动
    ProcessManager(ProcessManager &&)            = default;
    ProcessManager &operator=(ProcessManager &&) = default;

    /**
     * @brief 创建并添加进程
     * @param cmd 命令行参数
     * @param working_dir 工作目录
     * @param redirect 重定向配置
     * @return 进程指针
     */
    Ptr create_process(std::vector<std::string> cmd,
                       std::string              working_dir = "",
                       RedirectConfig           redirect    = RedirectConfig {})
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto process = std::make_shared<Process>(std::move(cmd), std::move(working_dir), std::move(redirect));
        processes_.push_back(process);

        return process;
    }

    /**
     * @brief 启动指定进程
     * @param process 进程指针
     */
    void start_process(const Ptr &process)
    {
        if (!process)
        {
            throw ProcessException("Null process pointer");
        }
        process->start();
    }

    /**
     * @brief 启动所有进程
     */
    void start_all()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto &process : processes_)
        {
            if (process && process->get_state() == ProcessState::NotStarted)
            {
                try
                {
                    process->start();
                }
                catch (const ProcessException &e)
                {
                    // 记录错误但继续启动其他进程
                    errors_.push_back("Failed to start process: " + std::string(e.what()));
                }
            }
        }
    }

    /**
     * @brief 停止指定进程
     * @param process 进程指针
     * @param force 是否强制终止
     * @return 是否成功
     */
    bool stop_process(const Ptr &process, bool force = false)
    {
        if (!process)
        {
            return false;
        }

        try
        {
            if (force)
            {
                return process->kill();
            }
            else
            {
                return process->terminate();
            }
        }
        catch (const ProcessException &e)
        {
            return false;
        }
    }

    /**
     * @brief 停止所有进程
     * @param force 是否强制终止
     */
    void stop_all(bool force = false)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto &process : processes_)
        {
            if (process && process->is_running())
            {
                try
                {
                    if (force)
                    {
                        process->kill();
                    }
                    else
                    {
                        process->terminate();
                    }
                }
                catch (const ProcessException &e)
                {
                    errors_.push_back("Failed to stop process: " + std::string(e.what()));
                }
            }
        }
    }

    /**
     * @brief 等待指定进程结束
     * @param process 进程指针
     * @return 退出码
     */
    int wait_for_process(const Ptr &process)
    {
        if (!process)
        {
            throw ProcessException("Null process pointer");
        }
        return process->wait();
    }

    /**
     * @brief 等待所有进程结束
     */
    void wait_for_all()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto &process : processes_)
        {
            if (process && process->is_running())
            {
                try
                {
                    process->wait();
                }
                catch (const ProcessException &e)
                {
                    errors_.push_back("Failed to wait for process: " + std::string(e.what()));
                }
            }
        }
    }

    /**
     * @brief 根据PID查找进程
     * @param pid 进程ID
     * @return 进程指针（空指针表示未找到）
     */
    Ptr find_process(pid_t pid)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto &process : processes_)
        {
            if (process && process->get_pid() == pid)
            {
                return process;
            }
        }

        return nullptr;
    }

    /**
     * @brief 获取所有进程
     * @return 进程列表
     */
    std::vector<Ptr> get_all_processes()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return processes_;
    }

    /**
     * @brief 获取运行中的进程数量
     * @return 运行中进程数
     */
    size_t get_running_count()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        return std::count_if(processes_.begin(), processes_.end(), [](const Ptr &p) {
            return p && p->is_running();
        });
    }

    /**
     * @brief 清理已结束的进程
     */
    void cleanup()
    {
        std::lock_guard<std::mutex> lock(mutex_);

        processes_.erase(std::remove_if(processes_.begin(), processes_.end(),
                                        [](const Ptr &p) {
                                            return !p || p->get_state() != ProcessState::Running;
                                        }),
                         processes_.end());

        // 清空错误列表
        errors_.clear();
    }

    /**
     * @brief 获取错误信息
     * @return 错误列表
     */
    std::vector<std::string> get_errors() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return errors_;
    }

private:
    std::vector<Ptr>         processes_;
    std::vector<std::string> errors_;
    mutable std::mutex       mutex_;
};

NAMESPACE_END

#endif  // PROCESS_MANAGER_HPP
