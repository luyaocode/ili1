// 全局按键
#include "keyblocker.h"
#include <QMutex>
#include <iostream>
#include <thread>
#include "globaltool.h"

#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>

KeyMonitorTask::KeyMonitorTask(const std::vector<KeyWithModifier> &keys)
    : m_listKeys(keys), m_display(nullptr), m_rootWindow(0), m_stop(0)
{
}

KeyMonitorTask::~KeyMonitorTask()
{
    // 任务销毁时确保资源释放
    if (m_display)
    {
        XUngrabKeyboard(m_display, CurrentTime);
        XCloseDisplay(m_display);
        m_display = nullptr;
    }
}

bool KeyMonitorTask::init()
{
    m_display = XOpenDisplay(nullptr);
    if (!m_display)
    {
        m_error = "打开 X11 显示失败";
        return false;
    }

    m_rootWindow = DefaultRootWindow(m_display);
    grabKeys(m_listKeys);
    return true;
}

QString KeyMonitorTask::initError() const
{
    return m_error;
}

void KeyMonitorTask::stop()
{
    m_stop.store(1);  // 原子操作，线程安全
}

KeyMonitorSignaler *KeyMonitorTask::signaler()
{
    return &m_signaler;
}

void KeyMonitorTask::run()
{
    if (!m_display)
    {
        emit m_signaler.sigErrorOccurred("X11 资源未初始化");
        emit m_signaler.sigFinished();
        return;
    }

    XEvent          event;
    KeyWithModifier targetKey;
    while (m_stop.load() == 0)
    {
        // 非阻塞获取事件（使用XCheckMaskEvent，仅获取按键相关事件）
        bool hasEvent = XCheckMaskEvent(m_display, KeyReleaseMask, &event);
        if (!hasEvent)
        {
            QThread::msleep(10);
            continue;
        }
        bool isMonitoredKey = false;
        // 提取当前事件的键码和修饰码
        KeyCode      currentCode = event.xkey.keycode;
        unsigned int currentMod  = event.xkey.state;
        std::cout << "[CommonTool] key event: " << getKeyEventString(event) << std::endl;
        // 过滤无关修饰符（如CapsLock/NumLock，避免干扰判断）
        currentMod &= (ShiftMask | ControlMask | Mod1Mask | Mod4Mask);
        // 对比目标列表，判断是否为需要抓取的按键
        for (const auto &target : m_listKeys)
        {
            // 键码必须完全匹配，修饰码必须包含目标修饰符（允许额外修饰符）
            KeyCode targetKeyCode = XKeysymToKeycode(m_display, target.keysym);
            if (currentCode == targetKeyCode && (currentMod & target.modifier) == target.modifier)
            {
                isMonitoredKey = true;
                targetKey      = target;
                break;
            }
        }
        if (isMonitoredKey)
        {
            unsigned long eventMask = KeyReleaseMask;
            XCheckWindowEvent(m_display, m_rootWindow, eventMask, &event);
            if (targetKey.callback)
            {
                targetKey.callback();
            }
            else
            {
                emit m_signaler.sigBlocked();
            }
        }
    }

    // 清理：取消窗口事件监听
    XSelectInput(m_display, m_rootWindow, 0);
    emit m_signaler.sigFinished();
}

// 强制释放指定按键的所有抓取（核心：遍历所有窗口）
void KeyMonitorTask::forceUngrabKey(KeyCode keycode, unsigned int modifier)
{
    if (!m_display || m_rootWindow == None)
        return;

    // 1. 遍历根窗口下的所有子窗口，强制释放
    Window       root, parent, *children = nullptr;
    unsigned int nchildren = 0;
    if (XQueryTree(m_display, m_rootWindow, &root, &parent, &children, &nchildren))
    {
        for (unsigned int i = 0; i < nchildren; ++i)
        {
            // 释放当前窗口对该按键的抓取
            XUngrabKey(m_display, keycode, modifier, children[i]);
            // 额外释放AnyModifier的情况（防止漏抓）
            XUngrabKey(m_display, keycode, AnyModifier, children[i]);
        }
        if (children)
            XFree(children);
    }

    // 2. 释放根窗口的抓取
    XUngrabKey(m_display, keycode, modifier, m_rootWindow);
    XUngrabKey(m_display, keycode, AnyModifier, m_rootWindow);

    // 3. 强制刷新X11请求（确保释放操作生效）
    XFlush(m_display);
    // 短暂延迟（避免X11服务器未处理完释放请求）
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

void KeyMonitorTask::grabKeys(const std::vector<KeyWithModifier> &targetKeys)
{
    if (!m_display)
    {
        return;
    }
    for (const auto &key : targetKeys)
    {
        // 调用XGrabKey抓取单个按键
        KeyCode keycode = XKeysymToKeycode(m_display, key.keysym);

        // 在 grabKeys 函数中，对每个目标按键，抓取所有修饰符组合变体
        std::vector<unsigned int> modVariants = {
            key.modifier,             // 原始组合：Ctrl+Shift
            key.modifier | Mod2Mask,  // Ctrl+Shift+NumLock（排除NumLock干扰）
            key.modifier | LockMask,  // Ctrl+Shift+CapsLock（排除CapsLock干扰）
            key.modifier | Mod5Mask   // 兼容部分键盘的修饰符变体
        };
        for (auto mod : modVariants)
        {
            int result = XGrabKey(m_display,
                                  keycode,        // 用户输入的键码
                                  mod,            // 用户输入的修饰码
                                  m_rootWindow,   // 全局监听（根窗口）
                                  False,          // owner_events：不向原窗口发送事件
                                  GrabModeAsync,  // 本地非阻塞模式
                                  GrabModeAsync   // 服务器非阻塞模式
            );
            if (result == GrabSuccess)
            {
                std::cout << "成功抓取按键： " << keyCombinationToString(mod, key.keysym) << std::endl;
            }
        }
    }
}

KeyBlocker::KeyBlocker(const std::vector<KeyWithModifier> &keys): m_listKeys(keys), m_task(nullptr)
{
}

void KeyBlocker::reset(const std::vector<KeyWithModifier> &keys)
{
    m_listKeys = keys;
}

KeyBlocker::~KeyBlocker()
{
    stop();  // 析构时停止任务
}

void KeyBlocker::start()
{
    stop();  // 先停止已有任务

    // 创建并初始化任务
    m_task = new KeyMonitorTask(m_listKeys);
    for (auto key : m_listKeys)
    {
        std::cout << "开始监控" << key.toString().toStdString() << std::endl;
    }

    if (!m_task->init())
    {
        QString error = m_task->initError();
        delete m_task;
        m_task = nullptr;
        emit sigErrorOccurred(error);
        return;
    }

    // 连接信号（通过信号转发器）
    connect(m_task->signaler(), &KeyMonitorSignaler::sigErrorOccurred, this, &KeyBlocker::sigErrorOccurred);
    connect(m_task->signaler(), &KeyMonitorSignaler::sigFinished, this, &KeyBlocker::slotTaskFinished);
    connect(m_task->signaler(), &KeyMonitorSignaler::sigBlocked, this, &KeyBlocker::sigBlocked);

    // 提交任务到线程池
    QThreadPool::globalInstance()->start(m_task);
    emit sigStarted();
}

void KeyBlocker::stop()
{
    if (!m_task)
        return;

    // 通知任务停止
    m_task->stop();

    // 等待任务结束（最多 1 秒）
    if (!QThreadPool::globalInstance()->waitForDone(1000))
    {
        emit sigErrorOccurred("键盘监控任务未正常退出，强制终止");
        // 线程池无法直接终止任务，此处标记任务失效
    }

    m_task = nullptr;  // 任务会被线程池自动销毁
    emit sigStopped();
}

void KeyBlocker::slotTaskFinished()
{
    m_task = nullptr;
    emit sigStopped();
}

QString KeyWithModifier::toString() const
{
    return QString::fromStdString(keyCombinationToString(modifier, keysym));
}
