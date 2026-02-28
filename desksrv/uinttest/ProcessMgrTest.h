#ifndef PROCESSMGRTEST_H
#define PROCESSMGRTEST_H
#include "unify/processmanager.hpp"
#include <QTest>
#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QCoreApplication>
#include <unistd.h>
#include <chrono>
#include <thread>

class ProcessTask : public QObject
{
    Q_OBJECT

public:
    ProcessTask(unify::ProcessManager *manager, int index, QObject *parent = nullptr);

public slots:
    void run();

signals:
    void finished();

private:
    unify::ProcessManager *m_manager;  ///< 进程管理器指针（非所有权）
    int                    m_index;    ///< 线程索引
};

class ProcessMgrTest
{
public:
    // 初始化（每个测试函数执行前）
    void init();

    // 清理（每个测试函数执行后）
    void cleanup();

    // 核心测试函数：覆盖所有进程管理器功能
    void test_processManager();

private:
    // 辅助函数：检查文件是否包含指定内容
    bool fileContains(const QString &filepath, const QString &content);

    // 辅助函数：等待条件满足（带超时）
    template<typename Func>
    bool waitForCondition(Func &&func, int timeoutSeconds = 5)
    {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count() <
               timeoutSeconds)
        {
            if (func())
            {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return false;
    }
};

#endif  // PROCESSMGRTEST_H
