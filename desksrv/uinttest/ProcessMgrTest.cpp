#include "ProcessMgrTest.h"

void ProcessMgrTest::init()
{
    QFile::remove("test_stdout.log");
    QFile::remove("test_stderr.log");
    QFile::remove("test_stdin.txt");
}

void ProcessMgrTest::cleanup()
{
    QFile::remove("test_stdout.log");
    QFile::remove("test_stderr.log");
    QFile::remove("test_stdin.txt");
}

void ProcessMgrTest::test_processManager()
{
    using namespace unify;

    /************************** 1. 基础功能测试 **************************/
    // 1.1 测试空管理器状态
    {
        ProcessManager manager;
        QCOMPARE(manager.get_all_processes().size(), static_cast<size_t>(0));
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(0));
        QCOMPARE(manager.get_errors().size(), static_cast<size_t>(0));
        QVERIFY(manager.find_process(-1) == nullptr);
    }

    // 1.2 测试创建单个进程（未启动状态）
    {
        ProcessManager manager;
        auto           proc = manager.create_process({"echo", "hello world"});

        QVERIFY(proc != nullptr);
        QCOMPARE(proc->get_pid(), static_cast<pid_t>(-1));  // 显式转换为 pid_t
        QCOMPARE(proc->get_state(), ProcessState::NotStarted);
        QCOMPARE(proc->get_exit_code(), 0);
        QCOMPARE(proc->get_start_time(), static_cast<uint64_t>(0));    // 匹配 uint64_t 类型
        QCOMPARE(proc->get_running_time(), static_cast<uint64_t>(0));  // 匹配 uint64_t 类型
        QVERIFY(!proc->is_running());
        QCOMPARE(manager.get_all_processes().size(), static_cast<size_t>(1));  // size_t 类型匹配
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(0));         // size_t 类型匹配
    }

    // 1.3 测试启动/等待单个进程（正常退出）
    {
        ProcessManager manager;
        auto           proc = manager.create_process({"echo", "test_single_process"});

        // 启动进程
        manager.start_process(proc);
        QVERIFY(proc->is_running());
        QVERIFY(proc->get_pid() != static_cast<pid_t>(-1));
        QVERIFY(proc->get_start_time() != static_cast<uint64_t>(0));
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(1));

        // 等待进程结束
        int exit_code = manager.wait_for_process(proc);
        QCOMPARE(exit_code, 0);
        QCOMPARE(proc->get_state(), ProcessState::Exited);
        QCOMPARE(proc->get_exit_code(), 0);
        QVERIFY(proc->get_end_time() != static_cast<uint64_t>(0));
//        QVERIFY(proc->get_running_time() >= static_cast<uint64_t>(0));
        QVERIFY(!proc->is_running());
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(0));
    }

    /************************** 2. 批量操作测试 **************************/
    // 2.1 测试批量创建/启动/等待多个进程
    {
        ProcessManager manager;

        // 创建3个进程
        auto proc1 = manager.create_process({"sleep", "1"});
        auto proc2 = manager.create_process({"echo", "proc2"});
        auto proc3 = manager.create_process({"ls", "-l"});

        QCOMPARE(manager.get_all_processes().size(), static_cast<size_t>(3));
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(0));

        // 批量启动
        manager.start_all();
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(3));

        // 批量等待
        manager.wait_for_all();
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(0));

        // 验证所有进程都已退出
        for (auto &proc : manager.get_all_processes())
        {
            QCOMPARE(proc->get_state(), ProcessState::Exited);
            QCOMPARE(proc->get_exit_code(), 0);
        }
    }

    // 2.2 测试批量停止进程
    {
        ProcessManager manager;

        // 创建长时间运行的进程
        auto proc1 = manager.create_process({"sleep", "10"});
        auto proc2 = manager.create_process({"sleep", "10"});

        manager.start_all();
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(2));

        // 优雅停止所有进程
        manager.stop_all(false);
        bool allStopped = waitForCondition([&]() {
            return manager.get_running_count() == 0;
        });
        QVERIFY(allStopped);

        // 验证进程状态（被杀死）
        QCOMPARE(proc1->get_state(), ProcessState::Killed);
        QCOMPARE(proc2->get_state(), ProcessState::Killed);
    }

    // 2.3 测试强制终止进程
    {
        ProcessManager manager;
        auto           proc = manager.create_process({"sleep", "10"});

        manager.start_process(proc);
        QVERIFY(proc->is_running());

        // 强制kill进程
        bool killed = manager.stop_process(proc, true);
        QVERIFY(killed);

        bool stopped = waitForCondition([&]() {
            return !proc->is_running();
        });
        QVERIFY(stopped);
        QCOMPARE(proc->get_state(), ProcessState::Killed);
    }

    /************************** 3. 文件重定向测试 **************************/
    // 3.1 测试标准输出/错误重定向
    {
        ProcessManager manager;

        // 配置重定向
        RedirectConfig redirect;
        redirect.stdout_path = "test_stdout.log";
        redirect.stderr_path = "test_stderr.log";
        redirect.append      = true;

        // 创建输出内容的进程
        auto proc = manager.create_process({"sh", "-c", "echo 'stdout_test' && echo 'stderr_test' >&2"}, "", redirect);

        manager.start_process(proc);
        manager.wait_for_process(proc);

        // 验证重定向文件内容
        QVERIFY(fileContains("test_stdout.log", "stdout_test"));
        QVERIFY(fileContains("test_stderr.log", "stderr_test"));
    }

    // 3.2 测试标准输入重定向
    {
        // 先创建输入文件
        QFile infile("test_stdin.txt");
        if (infile.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QTextStream out(&infile);
            out << "input_test_content";
            infile.close();
        }

        ProcessManager manager;
        RedirectConfig redirect;
        redirect.stdin_path  = "test_stdin.txt";
        redirect.stdout_path = "test_stdout.log";

        // 读取stdin并输出到stdout的进程
        auto proc = manager.create_process({"cat"},  // cat 命令默认读取stdin并输出到stdout
                                           "", redirect);

        manager.start_process(proc);
        manager.wait_for_process(proc);

        // 验证输入被正确读取并输出
        QVERIFY(fileContains("test_stdout.log", "input_test_content"));
    }

    /************************** 4. 边界条件与异常测试 **************************/
    // 4.1 测试启动不存在的命令（进程创建失败）
    {
        ProcessManager manager;
        auto           proc = manager.create_process({"non_existent_command_123456"});

        // 启动进程（fork 成功，execvp 失败）
        manager.start_process(proc);
        QVERIFY(proc->is_running());  // 此时状态为 Running（fork 成功）

        // 主动 wait 子进程，感知 execvp 失败
        int exitCode = manager.wait_for_process(proc);

        // 验证结果：子进程退出，状态为 Exited/Killed，退出码非0
        QVERIFY(proc->get_state() == ProcessState::Exited || proc->get_state() == ProcessState::Crashed);
        QVERIFY(exitCode != 0);  // execvp 失败后子进程退出码为非0
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(0));
    }

    // 4.2 测试重复启动同一进程
    {
        ProcessManager manager;
        auto           proc = manager.create_process({"echo", "repeat_start"});

        // 第一次启动成功
        manager.start_process(proc);
        manager.wait_for_process(proc);

        // 第二次启动应该抛出异常
        bool exceptionThrown = false;
        try
        {
            manager.start_process(proc);
        }
        catch (const ProcessException &)
        {
            exceptionThrown = true;
        }
        QVERIFY(exceptionThrown);

        QCOMPARE(proc->get_state(), ProcessState::Exited);
    }

    // 4.3 测试停止未启动的进程
    {
        ProcessManager manager;
        auto           proc = manager.create_process({"sleep", "5"});

        // 停止未启动的进程应返回false
        bool stopped = manager.stop_process(proc);
        QVERIFY(!stopped);
        QCOMPARE(proc->get_state(), ProcessState::NotStarted);
    }

    // 4.4 测试查找不存在的PID
    {
        ProcessManager manager;
        auto           proc = manager.create_process({"echo", "find_test"});
        manager.start_process(proc);
        manager.wait_for_process(proc);

        // 查找有效PID
        QCOMPARE(manager.find_process(proc->get_pid()), proc);
        // 查找无效PID
        QVERIFY(manager.find_process(999999) == nullptr);
    }

    // 4.5 测试清理已结束的进程
    {
        ProcessManager manager;

        // 创建并启动3个进程
        manager.create_process({"echo", "clean1"})->start();
        manager.create_process({"echo", "clean2"})->start();
        auto runningProc = manager.create_process({"sleep", "10"});
        runningProc->start();

        // 等待前两个短进程结束
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // 清理前状态
        QCOMPARE(manager.get_all_processes().size(), static_cast<size_t>(3));

        // 执行清理
        manager.cleanup();

        // 清理后：仅保留运行中的进程
        QCOMPARE(manager.get_all_processes().size(), static_cast<size_t>(1));
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(1));

        // 停止剩余进程
        manager.stop_all(true);
    }

    // 4.6 测试空指针操作异常
    {
        ProcessManager manager;

        // 启动空进程指针（应抛异常）
        bool startException = false;
        try
        {
            manager.start_process(nullptr);
        }
        catch (const ProcessException &)
        {
            startException = true;
        }
        QVERIFY(startException);

        // 等待空进程指针（应抛异常）
        bool waitException = false;
        try
        {
            manager.wait_for_process(nullptr);
        }
        catch (const ProcessException &)
        {
            waitException = true;
        }
        QVERIFY(waitException);

        // 停止空进程指针（应返回false，不抛异常）
        QVERIFY(!manager.stop_process(nullptr));

        QCOMPARE(manager.get_errors().size(), static_cast<size_t>(0));
    }

    // 4.7 测试进程异常崩溃场景
    {
        ProcessManager manager;
        // 执行会崩溃的命令（非法指令）
        auto proc = manager.create_process({"sh", "-c", "kill -SEGV $$"});

        manager.start_process(proc);
        manager.wait_for_process(proc);

        // 验证进程状态为崩溃/被信号终止
        QVERIFY(proc->get_state() == ProcessState::Killed || proc->get_state() == ProcessState::Crashed);
        QVERIFY(proc->get_exit_code() != 0);
    }

    /************************** 5. 线程安全测试 **************************/
    // 5.1 多线程并发操作进程管理器
    {
        ProcessManager       manager;
        QList<QThread *>     threads;
        QList<ProcessTask *> tasks;  // 保存任务对象避免提前析构

        // 启动10个线程并发创建/启动进程
        for (int i = 0; i < 10; ++i)
        {
            // 创建线程和任务对象
            QThread     *thread = new QThread;
            ProcessTask *task   = new ProcessTask(&manager, i);

            // 将任务对象移到线程中
            task->moveToThread(thread);

            // 连接信号槽：保证线程安全退出和资源清理
            QObject::connect(thread, &QThread::started, task, &ProcessTask::run);
            QObject::connect(task, &ProcessTask::finished, thread, &QThread::quit);
            QObject::connect(task, &ProcessTask::finished, task, &ProcessTask::deleteLater);
            QObject::connect(thread, &QThread::finished, thread, &QThread::deleteLater);

            // 保存线程对象（用于后续等待）
            threads.append(thread);
            tasks.append(task);

            // 启动线程
            thread->start();
        }

        // 等待所有线程完成
        for (QThread *thread : threads)
        {
            thread->wait();  // 阻塞等待线程退出
        }

        // 验证结果：所有进程都已结束
        QCOMPARE(manager.get_running_count(), static_cast<size_t>(0));
        QCOMPARE(manager.get_all_processes().size(), static_cast<size_t>(10));
        for (auto &proc : manager.get_all_processes())
        {
            QCOMPARE(proc->get_state(), ProcessState::Exited);
        }

        // 手动清理剩余对象（双重保障）
        qDeleteAll(threads);
        threads.clear();
        tasks.clear();
    }

    qInfo() << "All process manager test cases passed!";
}

bool ProcessMgrTest::fileContains(const QString &filepath, const QString &content)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream in(&file);
    QString     contentAll = in.readAll();
    file.close();

    return contentAll.contains(content);
}

ProcessTask::ProcessTask(unify::ProcessManager *manager, int index, QObject *parent)
    : QObject(parent), m_manager(manager), m_index(index)
{
    // 空构造
}

void ProcessTask::run()
{
    try
    {
        // 构造测试命令：输出线程索引
        std::string cmd = "echo thread_" + std::to_string(m_index);
        // 创建进程（执行shell命令）
        auto proc = m_manager->create_process({"sh", "-c", cmd});
        // 启动进程
        m_manager->start_process(proc);
        // 等待进程执行完成
        m_manager->wait_for_process(proc);
    }
    catch (const unify::ProcessException &e)
    {
        // 捕获并输出异常，避免线程崩溃
        qWarning() << QString("ProcessTask[%1] error: %2").arg(m_index).arg(e.what());
    }

    // 发送任务完成信号，通知线程退出
    emit finished();
}
