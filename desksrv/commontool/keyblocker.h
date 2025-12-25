#ifndef KEYBLOCKER_H
#define KEYBLOCKER_H

#include <QObject>
#include <QRunnable>
#include <QThreadPool>
#include <QMutex>
#include <QAtomicInt>
#include <vector>
#include <functional>
#include <X11/Xlib.h>
#include <X11/X.h>
#include "commontool_global.h"

// 定义键码+修饰码的结构体（用户输入的格式）
struct KeyWithModifier
{
    KeySym                keysym;    // 按键的键码（用户输入）
    unsigned int          modifier;  // 修饰码（如ControlMask、ShiftMask等，用户输入）
    std::function<void()> callback;
    KeyWithModifier(KeySym ks = 0, unsigned int mod = 0, std::function<void()> cb = nullptr)
        : keysym(ks), modifier(mod), callback(cb)
    {
    }
    QString toString() const;
};
Q_DECLARE_METATYPE(KeyWithModifier)
Q_DECLARE_METATYPE(std::vector<KeyWithModifier>)

// 信号转发器：用于在 QRunnable 中发送信号（QRunnable 本身不支持信号槽）
class KeyMonitorSignaler : public QObject
{
    Q_OBJECT
signals:
    void sigErrorOccurred(const QString &msg);    // 错误信息
    void sigFinished();                           // 任务结束
    void sigBlocked();  //拦截到了
};

// QRunnable 任务类
class COMMONTOOLSHARED_EXPORT KeyMonitorTask : public QRunnable
{
public:
    KeyMonitorTask(const std::vector<KeyWithModifier> &keys);

    ~KeyMonitorTask();

    // 初始化 X11 资源（必须在主线程调用）
    bool init();

    // 获取初始化错误信息
    QString initError() const;

    // 触发任务停止
    void stop();

    // 获取信号转发器（用于连接信号）
    KeyMonitorSignaler *signaler();

protected:
    // 任务主逻辑（在子线程执行）
//    void originrun();
    void run() override;

private:
    void forceUngrabKey(KeyCode keycode, unsigned int modifier);
    void grabKeys(const std::vector<KeyWithModifier> &targetKeys);

private:
    std::vector<KeyWithModifier> m_listKeys;
    Display                     *m_display    = nullptr;  // X11 显示句柄
    Window                       m_rootWindow = 0;        // 根窗口
    QAtomicInt                   m_stop;                  // 停止标志（原子变量，线程安全）
    QString                      m_error;                 // 初始化错误信息
    KeyMonitorSignaler           m_signaler;              // 信号转发器
};

// 对外接口类：管理任务生命周期
class KeyBlocker : public QObject
{
    Q_OBJECT
public:
    explicit KeyBlocker(const std::vector<KeyWithModifier> &keys);
    void reset(const std::vector<KeyWithModifier> &keys);

    virtual ~KeyBlocker();

public slots:
    // 启动拦截
    void start();

    // 停止拦截
    void stop();

signals:
    void sigStarted();                          // 拦截已启动
    void sigStopped();                          // 拦截已停止
    void sigBlocked();  // 已拦截
    void sigErrorOccurred(const QString &msg);  // 错误信息

private slots:
    // 任务结束时触发
    void slotTaskFinished();

private:
    std::vector<KeyWithModifier> m_listKeys;
    KeyMonitorTask              *m_task = nullptr;  // 当前任务（由线程池管理生命周期）
};

#endif  // KEYBLOCKER_H
