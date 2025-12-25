#include "inputwatcher.h"
#include <QDebug>
#include <QThread>
#include "x11struct.h"

const int MOVE_THRESHOLD = 3;

InputWatcher::InputWatcher(QObject *parent): QObject(parent), m_isMonitoring(false), m_x11st(new x11struct)
{
    // 初始化 X11 连接
    m_x11st->m_x11Display = XOpenDisplay(nullptr);
    if (!m_x11st->m_x11Display)
    {
        qCritical() << "[InputWatcher] 无法连接到 X Server！请确保当前环境有图形界面";
        return;
    }

    // 获取系统根窗口
    m_x11st->m_x11RootWindow = DefaultRootWindow(m_x11st->m_x11Display);
    qDebug() << "[InputWatcher] 成功连接 X Server，根窗口 ID:" << m_x11st->m_x11RootWindow;
}

InputWatcher::~InputWatcher()
{
    stopWatching();

    // 关闭 X11 连接
    if (m_x11st->m_x11Display)
    {
        XCloseDisplay(m_x11st->m_x11Display);
        m_x11st->m_x11Display = nullptr;
        qDebug() << "[InputWatcher] X Server 连接已关闭";
    }
}

bool InputWatcher::startWatching()
{
    QMutexLocker locker(&m_monitorMutex);
    if (m_isMonitoring)
    {
        qWarning() << "[InputWatcher] 监控已在运行，无需重复启动";
        return true;
    }

    // 检查 X11 连接是否有效
    if (!m_x11st->m_x11Display)
    {
        qCritical() << "[InputWatcher] X Server 连接无效，无法启动监控";
        emit monitoringStarted(false);
        return false;
    }

    // 创建事件监听线程
    int threadRet = pthread_create(&m_eventThread, nullptr, x11EventLoop, this);
    if (threadRet != 0)
    {
        qCritical() << "[InputWatcher] 创建事件线程失败！错误码:" << threadRet;
        emit monitoringStarted(false);
        return false;
    }

    // 更新监控状态
    m_isMonitoring     = true;
    m_lastActivityTime = QDateTime::currentDateTime();
    qDebug() << "[InputWatcher] 监控已启动，开始监听全局输入事件";
    emit monitoringStarted(true);
    return true;
}

void InputWatcher::stopWatching()
{
    QMutexLocker locker(&m_monitorMutex);
    if (!m_isMonitoring)
    {
        qWarning() << "[InputWatcher] 监控未运行，无需停止";
        return;
    }

    // 标记监控状态为停止
    m_isMonitoring = false;

    // 发送空事件唤醒阻塞的 XNextEvent
    XEvent dummyEvent;
    memset(&dummyEvent, 0, sizeof(XEvent));
    dummyEvent.type                 = ClientMessage;
    dummyEvent.xclient.window       = m_x11st->m_x11RootWindow;
    dummyEvent.xclient.message_type = None;
    dummyEvent.xclient.format       = 32;
    XSendEvent(m_x11st->m_x11Display, m_x11st->m_x11RootWindow, False, 0, &dummyEvent);
    XFlush(m_x11st->m_x11Display);

    // 等待线程退出
    struct timespec timeout   = {5, 0};
    int             threadRet = pthread_timedjoin_np(m_eventThread, nullptr, &timeout);
    if (threadRet == 0)
    {
        qDebug() << "[InputWatcher] 事件线程已正常退出";
    }
    else if (threadRet == ETIMEDOUT)
    {
        qWarning() << "[InputWatcher] 等待线程退出超时，强制终止";
        pthread_cancel(m_eventThread);
    }
    else
    {
        qCritical() << "[InputWatcher] 线程退出失败！错误码:" << threadRet;
    }

    // 取消 X11 事件注册
    XSelectInput(m_x11st->m_x11Display, m_x11st->m_x11RootWindow, 0);
    XFlush(m_x11st->m_x11Display);

    emit monitoringStopped();
    qDebug() << "[InputWatcher] 监控已停止";
}

QDateTime InputWatcher::lastActivityTime() const
{
    // 使用const_cast移除const属性
    QMutexLocker locker(const_cast<QMutex *>(&m_timeMutex));
    return m_lastActivityTime;
}

bool InputWatcher::isUserActiveInLastSeconds(int sec)
{
    QMutexLocker locker(const_cast<QMutex *>(&m_timeMutex));
    // 获取当前时间
    QDateTime currentTime = QDateTime::currentDateTime();

    // 计算两个时间点之间的秒数差
    // 注意：如果lastTime在currentTime之后，会返回负数
    qint64 secondsDiff = m_lastActivityTime.secsTo(currentTime);

    // 判断差值是否在0到sec秒之间（包含sec秒）
    return (secondsDiff >= 0 && secondsDiff <= sec);
}

// 检查 X11 显示连接是否有效
bool isDisplayValid(Display *display)
{
    if (!display)
        return false;

    // 通过尝试获取默认屏幕来验证连接
    int screen = DefaultScreen(display);
    if (screen < 0 || screen >= ScreenCount(display))
    {
        return false;
    }

    // 尝试获取根窗口（如果失败则连接无效）
    Window root = RootWindow(display, screen);
    return (root != None);
}

void *InputWatcher::x11EventLoop(void *arg)
{
    int ret = pthread_setname_np(pthread_self(), "thread_inputwatcher");
    if (ret != 0)
    {
        // 名称设置失败（通常是长度超限）
        // 可通过strerror(ret)获取具体错误信息
    }
    InputWatcher *monitor = static_cast<InputWatcher *>(arg);
    if (!monitor || !monitor->m_x11st->m_x11Display)
    {
        qCritical() << "[InputWatcher] 线程初始化失败，参数无效";
        return nullptr;
    }

    qDebug() << "[InputWatcher] 事件监听线程已启动";
    monitor->processX11Event();
    qDebug() << "[InputWatcher] 事件监听线程准备退出";
    return nullptr;
}

void InputWatcher::processX11Event()
{
    // 先获取初始位置
    int          x = 0, y = 0;
    int          prevX = 0, prevY = 0;
    int          rootX, rootY;
    unsigned int mask;
    Window       rootRet, childRet;
    bool         isUserActivity = false;
    QString      eventType;
    int queryResult = XQueryPointer(m_x11st->m_x11Display, m_x11st->m_x11RootWindow, &rootRet, &childRet, &rootX,
                                    &rootY, &x, &y, &mask);
    if (queryResult == 0)
    {
        qWarning() << "[InputWatcher] 首次查询鼠标位置失败，可能鼠标未激活";
        // 等待用户操作激活鼠标
        QThread::msleep(1000);
        return;
    }
    prevX = rootX;
    prevY = rootY;
    while (true)
    {
        isUserActivity = false;
        // 每次查询前检查 display 是否有效
        if (!isDisplayValid(m_x11st->m_x11Display))
        {
            qCritical() << "[InputWatcher] X11 连接已失效，退出监测";
            break;
        }
        // 再次查询鼠标位置
        queryResult = XQueryPointer(m_x11st->m_x11Display, m_x11st->m_x11RootWindow, &rootRet, &childRet, &rootX,
                                    &rootY, &x, &y, &mask);

        if (queryResult != 0)
        {  // 0 表示查询失败
            // 计算x和y方向的移动差值
            int deltaX = rootX - prevX;
            int deltaY = rootY - prevY;

            // 计算欧氏距离的平方（避免开方运算，提高效率）
            int distanceSquared = deltaX * deltaX + deltaY * deltaY;

            // 当移动距离超过阈值时，才判定为有效移动
            if (distanceSquared > MOVE_THRESHOLD * MOVE_THRESHOLD)
            {
                eventType = QDateTime::currentDateTime().toString("hh:mm:ss.zzz") + "鼠标移动: (" +
                            QString::number(prevX) + "," + QString::number(prevY) + ") → (" + QString::number(rootX) +
                            "," + QString::number(rootY) + ")";
//                qDebug() << eventType;
                prevX          = rootX;
                prevY          = rootY;
                isUserActivity = true;
            }
        }
        else
        {
            qWarning() << "鼠标位置查询失败，可能窗口已关闭";
        }
        // 处理用户活动
        if (isUserActivity)
        {
            QMutexLocker locker(&m_timeMutex);
            m_lastActivityTime = QDateTime::currentDateTime();
            emit userActivityDetected(eventType);
        }
        QThread::msleep(100);
    }
}
