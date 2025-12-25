#ifndef INPUTWATCHER_H
#define INPUTWATCHER_H

#include <QDataStream>
#include <QTextStream>
#include <QObject>
#include <QDateTime>
#include <QMutex>
#include <pthread.h>

struct x11struct;
class InputWatcher : public QObject
{
    Q_OBJECT
public:
    explicit InputWatcher(QObject *parent = nullptr);
    ~InputWatcher() override;

    // 启动/停止监控
    bool startWatching();
    void stopWatching();

    // 获取最后一次用户活动时间
    QDateTime lastActivityTime() const;

    // sec秒前是否有活动
    bool isUserActiveInLastSeconds(int sec);

signals:
    // 检测到用户活动时触发
    void userActivityDetected(const QString &eventType);
    // 监控状态变化
    void monitoringStarted(bool success);
    void monitoringStopped();

private:
    // 线程函数：循环读取 X11 事件
    static void* x11EventLoop(void *arg);
    // 处理单个 X11 事件
    void processX11Event();

private:
    bool m_isMonitoring;          // 监控状态标记
    QMutex m_monitorMutex;        // 线程安全锁
    QDateTime m_lastActivityTime; // 最后一次用户活动时间
    QMutex m_timeMutex;           // 时间变量的线程安全锁

    pthread_t m_eventThread;      // 事件监听线程 ID
    x11struct* m_x11st;
};

#endif // INPUTWATCHER_H
