#include "mousesimulator.h"
#include <QThread>
#include <QDebug>
#include "virtualmousewidget.h"
#include "x11struct.h"
#include "globaldef.h"

// ========== 新增：多屏必须的头文件 ==========
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/Xinerama.h>

// 静态单例初始化
MouseSimulator *MouseSimulator::m_instance = nullptr;
QMutex          MouseSimulator::m_mutex;

MouseSimulator::MouseSimulator(QObject *parent): QObject(parent)
{
    m_x11struct = new x11struct;
    // 连接X11服务器（默认DISPLAY=:0，支持自定义DISPLAY环境变量）
    const char *displayName   = getenv("DISPLAY");
    m_x11struct->m_x11Display = XOpenDisplay(displayName ? displayName : ":0");

    if (!m_x11struct->m_x11Display)
    {
        qCritical() << "[MouseSimulator] 连接X11服务器失败！请检查DISPLAY环境变量";
        return;
    }

    // 检查XTest扩展是否可用（输入模拟核心依赖）
    //    int xtestEventBase, xtestErrorBase;
    //    if (!XTestQueryExtension(m_x11struct->m_x11Display, &xtestEventBase, &xtestErrorBase)) {
    //        qCritical() << "[MouseSimulator] X11 XTest扩展未启用！请安装libxtst并启用扩展";
    //        XCloseDisplay(m_x11struct->m_x11Display);
    //        m_x11struct->m_x11Display = nullptr;
    //    }
    // 初始化虚拟鼠标绘制窗口
    //    m_virtualMouseWidget = new VirtualMouseWidget();
    //    qInfo() << "[MouseSimulator] 虚拟鼠标绘制窗口初始化成功";

    qInfo() << "[MouseSimulator] X11鼠标模拟器初始化成功";
}

MouseSimulator::~MouseSimulator()
{
    //    if (m_virtualMouseWidget) {
    //        m_virtualMouseWidget->close();
    //        m_virtualMouseWidget->deleteLater();
    //    }
    if (m_x11struct->m_x11Display)
    {
        XCloseDisplay(m_x11struct->m_x11Display);
        m_x11struct->m_x11Display = nullptr;
    }
    if (m_x11struct)
    {
        delete m_x11struct;
        m_x11struct = nullptr;
    }
    qInfo() << "[MouseSimulator] X11鼠标模拟器已释放";
}

MouseSimulator *MouseSimulator::getInstance()
{
    QMutexLocker lock(&m_mutex);
    if (!m_instance)
    {
        m_instance = new MouseSimulator();
    }
    return m_instance;
}

void MouseSimulator::deleteInstance()
{
    if (m_instance)
    {
        delete m_instance;
        m_instance = nullptr;
    }
}

bool MouseSimulator::moveMouse(int x, int y)
{
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct->m_x11Display)
        return false;

    // XTest模拟绝对移动（0号鼠标设备）
    XTestFakeMotionEvent(m_x11struct->m_x11Display, 0, x, y, CurrentTime);
    XFlush(m_x11struct->m_x11Display);  // 立即刷新事件队列，确保生效
    //    // 同步更新虚拟鼠标绘制位置
    //       if (m_virtualMouseWidget) {
    //           m_virtualMouseWidget->updateMousePos(x, y);
    //       }
    lock.unlock();
    LOG_MESSAGE("MouseSimulator", QString("鼠标移动到绝对坐标：%1,%2").arg(x).arg(y))
    return true;
}

// ========== 新增：多屏核心重载函数 → 屏幕索引 + 本地坐标 移动鼠标 ==========
bool MouseSimulator::moveMouse(int screenIdx, int localX, int localY)
{
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct->m_x11Display)
        return false;

    int globalX = 0, globalY = 0;
    // 本地坐标 → 全局坐标 转换
    localToGlobal(screenIdx, localX, localY, globalX, globalY);
    if (globalX < 0 || globalY < 0)
        return false;

    XTestFakeMotionEvent(m_x11struct->m_x11Display, 0, globalX, globalY, CurrentTime);
    XFlush(m_x11struct->m_x11Display);
    LOG_MESSAGE("MouseSimulator", QString("屏幕[%1]鼠标移动：本地(%2,%3) → 全局(%4,%5)")
                                      .arg(screenIdx)
                                      .arg(localX)
                                      .arg(localY)
                                      .arg(globalX)
                                      .arg(globalY))
    return true;
}

bool MouseSimulator::moveMouseRelative(int dx, int dy)
{
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct->m_x11Display)
        return false;

    // 先获取当前位置，再计算相对偏移
    int curX, curY;
    if (!getCurrentMousePos(curX, curY))
    {
        LOG_MESSAGE("MouseSimulator", "获取当前鼠标位置失败，无法相对移动")
        return false;
    }

    int  newX = curX + dx;
    int  newY = curY + dy;
    bool ret  = moveMouse(newX, newY);  // 复用绝对移动（自动同步绘制）
    return ret;
}

bool MouseSimulator::clickMouse(MouseButton button, int x, int y)
{
    // 先按下
    if (!pressMouse(button, x, y))
    {
        return false;
    }
    // 可选：添加短延迟（模拟真实点击的按下-释放间隔）
    QThread::msleep(50);
    // 再释放
    return releaseMouse(button, x, y);
}

// ========== 新增：多屏核心重载函数 → 屏幕索引 + 本地坐标 点击鼠标 ==========
bool MouseSimulator::clickMouse(MouseButton button, int screenIdx, int x, int y)
{
    if (!pressMouse(button, screenIdx, x, y))
        return false;
    QThread::msleep(50);
    return releaseMouse(button, screenIdx, x, y);
}

bool MouseSimulator::pressMouse(MouseSimulator::MouseButton button, int x, int y)
{
    if (button == MouseSimulator::MouseButton::ButtonNone)
    {
        return false;
    }
    // 校验X11连接有效性
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct || !m_x11struct->m_x11Display)
    {
        LOG_MESSAGE("MouseSimulator", "X11显示连接无效，无法按下鼠标")
        return false;
    }
    lock.unlock();
    // 如果指定了坐标，先移动到目标位置
    if (x >= 0 && y >= 0)
    {
        if (!moveMouse(x, y))
        {
            LOG_MESSAGE("MouseSimulator", QString("移动鼠标到坐标失败:%1,%2").arg(x).arg(y))
            return false;
        }
    }
    lock.relock();
    // 仅发送鼠标按下事件（不释放，实现长按）
    XTestFakeButtonEvent(m_x11struct->m_x11Display, button, True, CurrentTime);
    XFlush(m_x11struct->m_x11Display);
    lock.unlock();
    // 日志输出
    QString logMsg = QString("鼠标%1按下，坐标：%2,%3")
                         .arg(button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
                         .arg(x >= 0 ? QString::number(x) : "当前位置")
                         .arg(y >= 0 ? QString::number(y) : "当前位置");
    LOG_MESSAGE("MouseSimulator", logMsg)
    return true;
}

// ========== 新增：多屏核心重载函数 → 屏幕索引 + 本地坐标 按下鼠标 ==========
bool MouseSimulator::pressMouse(MouseButton button, int screenIdx, int x, int y)
{
    if (button == MouseSimulator::MouseButton::ButtonNone)
        return false;
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct || !m_x11struct->m_x11Display)
    {
        LOG_MESSAGE("MouseSimulator", "X11显示连接无效，无法按下鼠标")
        return false;
    }
    lock.unlock();

    if (x >= 0 && y >= 0)
    {
        if (!moveMouse(screenIdx, x, y))  // 调用多屏版moveMouse
        {
            LOG_MESSAGE("MouseSimulator", QString("屏幕[%1]移动鼠标到本地坐标失败:%2,%3").arg(screenIdx).arg(x).arg(y))
            return false;
        }
    }
    lock.relock();
    XTestFakeButtonEvent(m_x11struct->m_x11Display, button, True, CurrentTime);
    XFlush(m_x11struct->m_x11Display);
    lock.unlock();
    QString logMsg = QString("屏幕[%1]鼠标%2按下，本地坐标：%3,%4")
                         .arg(screenIdx)
                         .arg(button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
                         .arg(x >= 0 ? QString::number(x) : "当前位置")
                         .arg(y >= 0 ? QString::number(y) : "当前位置");
    LOG_MESSAGE("MouseSimulator", logMsg)
    return true;
}

bool MouseSimulator::releaseMouse(MouseSimulator::MouseButton button, int x, int y)
{
    if (button == MouseSimulator::MouseButton::ButtonNone)
    {
        return false;
    }
    // 校验X11连接有效性
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct || !m_x11struct->m_x11Display)
    {
        LOG_MESSAGE("MouseSimulator", "X11显示连接无效，无法按下鼠标");
        return false;
    }
    lock.unlock();

    // 可选：若指定坐标，先移动到目标位置（保证释放位置和按下位置一致）
    if (x >= 0 && y >= 0)
    {
        moveMouse(x, y);  // 即使移动失败，仍尝试释放按键（避免按键卡死）
    }

    // 仅发送鼠标释放事件
    lock.relock();
    XTestFakeButtonEvent(m_x11struct->m_x11Display, button, False, CurrentTime);
    XFlush(m_x11struct->m_x11Display);
    lock.unlock();
    // 日志输出
    QString logMsg = QString("鼠标%1按下，坐标：%2,%3")
                         .arg(button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
                         .arg(x >= 0 ? QString::number(x) : "当前位置")
                         .arg(y >= 0 ? QString::number(y) : "当前位置");
    LOG_MESSAGE("MouseSimulator", logMsg)
    return true;
}

// ========== 新增：多屏核心重载函数 → 屏幕索引 + 本地坐标 释放鼠标 ==========
bool MouseSimulator::releaseMouse(MouseButton button, int screenIdx, int x, int y)
{
    if (button == MouseSimulator::MouseButton::ButtonNone)
        return false;
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct || !m_x11struct->m_x11Display)
    {
        LOG_MESSAGE("MouseSimulator", "X11显示连接无效，无法释放鼠标");
        return false;
    }
    lock.unlock();

    if (x >= 0 && y >= 0)
        moveMouse(screenIdx, x, y);  // 调用多屏版moveMouse

    lock.relock();
    XTestFakeButtonEvent(m_x11struct->m_x11Display, button, False, CurrentTime);
    XFlush(m_x11struct->m_x11Display);
    lock.unlock();
    QString logMsg = QString("屏幕[%1]鼠标%2释放，本地坐标：%3,%4")
                         .arg(screenIdx)
                         .arg(button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
                         .arg(x >= 0 ? QString::number(x) : "当前位置")
                         .arg(y >= 0 ? QString::number(y) : "当前位置");
    LOG_MESSAGE("MouseSimulator", logMsg)
    return true;
}

bool MouseSimulator::doubleClickMouse(MouseButton button, int x, int y, int interval)
{
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct->m_x11Display)
        return false;
    lock.unlock();
    // 第一次点击
    clickMouse(button, x, y);
    // 双击间隔（模拟人类操作延迟）
    QThread::msleep(interval);
    // 第二次点击
    clickMouse(button, x, y);

    QString logMsg = QString("鼠标%1按下，坐标：%2,%3")
                         .arg(button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
                         .arg(x >= 0 ? QString::number(x) : "当前位置")
                         .arg(y >= 0 ? QString::number(y) : "当前位置");
    LOG_MESSAGE("MouseSimulator", logMsg)
    return true;
}

// ========== 新增：多屏核心重载函数 → 屏幕索引 + 本地坐标 双击鼠标 ==========
bool MouseSimulator::doubleClickMouse(MouseButton button, int screenIdx, int x, int y, int interval)
{
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct->m_x11Display)
        return false;
    lock.unlock();
    clickMouse(button, screenIdx, x, y);
    QThread::msleep(interval);
    clickMouse(button, screenIdx, x, y);
    QString logMsg = QString("屏幕[%1]鼠标%2双击，本地坐标：%3,%4")
                         .arg(screenIdx)
                         .arg(button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
                         .arg(x >= 0 ? QString::number(x) : "当前位置")
                         .arg(y >= 0 ? QString::number(y) : "当前位置");
    LOG_MESSAGE("MouseSimulator", logMsg)
    return true;
}

bool MouseSimulator::pressMouse(MouseButton button)
{
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct->m_x11Display)
        return false;

    XTestFakeButtonEvent((void *)m_x11struct->m_x11Display, button, True, CurrentTime);
    XFlush(m_x11struct->m_x11Display);
    //    // 同步更新虚拟鼠标按压状态
    //      if (m_virtualMouseWidget) {
    //          m_virtualMouseWidget->updateMousePressState(true);
    //      }
    lock.unlock();
    QString logMsg =
        QString("鼠标%1按下").arg(button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"));
    LOG_MESSAGE("MouseSimulator", logMsg)
    return true;
}

bool MouseSimulator::releaseMouse(MouseButton button)
{
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct->m_x11Display)
        return false;

    XTestFakeButtonEvent((void *)m_x11struct->m_x11Display, button, False, CurrentTime);
    XFlush(m_x11struct->m_x11Display);
    //    // 同步更新虚拟鼠标释放状态
    //    if (m_virtualMouseWidget) {
    //        m_virtualMouseWidget->updateMousePressState(false);
    //    }
    lock.unlock();
    QString logMsg =
        QString("鼠标%1按下").arg(button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"));
    LOG_MESSAGE("MouseSimulator", logMsg)
    return true;
}

bool MouseSimulator::scrollWheel(WheelDirection direction, int steps)
{
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct->m_x11Display)
        return false;

    // X11滚轮事件映射：Up/Down用Button4/5，Left/Right用Button6/7
    int wheelButton = 0;
    switch (direction)
    {
        case WheelUp:
            wheelButton = Button4;
            break;
        case WheelDown:
            wheelButton = Button5;
            break;
        case WheelLeft:
            wheelButton = Button6;
            break;
        case WheelRight:
            wheelButton = Button7;
            break;
        default:
            return false;
    }

    // 模拟多步滚动（每步按下+释放）
    for (int i = 0; i < steps; ++i)
    {
        XTestFakeButtonEvent((void *)m_x11struct->m_x11Display, wheelButton, True, CurrentTime);
        XTestFakeButtonEvent((void *)m_x11struct->m_x11Display, wheelButton, False, CurrentTime);
        QThread::msleep(10);  // 每步间隔，避免滚动过快
    }
    XFlush(m_x11struct->m_x11Display);
    lock.unlock();
    QString directionStr;
    switch (direction)
    {
        case WheelUp:
            directionStr = "上";
            break;
        case WheelDown:
            directionStr = "下";
            break;
        case WheelLeft:
            directionStr = "左";
            break;
        default:
            directionStr = "右";
            break;  // WheelRight
    }

    QString logMsg = QString("鼠标滚轮%1滚动%2步").arg(directionStr).arg(steps);
    LOG_MESSAGE("MouseSimulator", logMsg)
    return true;
}

bool MouseSimulator::getCurrentMousePos(int &x, int &y)
{
    QMutexLocker lock(&m_mutex);
    if (!m_x11struct->m_x11Display)
        return false;

    // 获取根窗口的鼠标位置
    Window       rootWindow = DefaultRootWindow(m_x11struct->m_x11Display);
    Window       childWindow;
    int          rootX, rootY;
    int          winX, winY;
    unsigned int mask;

    if (XQueryPointer(m_x11struct->m_x11Display, rootWindow, &rootWindow, &childWindow, &rootX, &rootY, &winX, &winY,
                      &mask))
    {
        x = rootX;
        y = rootY;
        return true;
    }
    lock.unlock();

    LOG_MESSAGE("MouseSimulator", "获取当前鼠标位置失败")
    return false;
}

// ======================================================================================
// ========== 核心新增：多屏工具函数 1 → 根据屏幕索引，获取屏幕的全局偏移+分辨率 QRect(x,y,w,h) ==========
// ======================================================================================
QRect MouseSimulator::getScreenRectByIndex(int screenIdx) const
{
    if (!m_x11struct->m_x11Display || screenIdx < 0)
        return QRect(-1, -1, -1, -1);

    QVector<QRect> screenRects;
    int            xrandrEventBase, xrandrErrorBase;

    // 优先级1：XRandR精准获取多屏信息（和ScreenShooter一致）
    if (XRRQueryExtension(m_x11struct->m_x11Display, &xrandrEventBase, &xrandrErrorBase))
    {
        Window              root = DefaultRootWindow(m_x11struct->m_x11Display);
        XRRScreenResources *res  = XRRGetScreenResources(m_x11struct->m_x11Display, root);
        if (res)
        {
            for (int i = 0; i < res->noutput; ++i)
            {
                XRROutputInfo *outputInfo = XRRGetOutputInfo(m_x11struct->m_x11Display, res, res->outputs[i]);
                if (!outputInfo || outputInfo->connection != RR_Connected || outputInfo->crtc == None)
                {
                    XRRFreeOutputInfo(outputInfo);
                    continue;
                }
                XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(m_x11struct->m_x11Display, res, outputInfo->crtc);
                if (crtcInfo)
                {
                    screenRects.append(QRect(crtcInfo->x, crtcInfo->y, crtcInfo->width, crtcInfo->height));
                    XRRFreeCrtcInfo(crtcInfo);
                }
                XRRFreeOutputInfo(outputInfo);
            }
            XRRFreeScreenResources(res);
        }
    }

    // 优先级2：Xinerama兜底
    if (screenRects.isEmpty() && XineramaIsActive(m_x11struct->m_x11Display))
    {
        int                 count   = 0;
        XineramaScreenInfo *screens = XineramaQueryScreens(m_x11struct->m_x11Display, &count);
        if (screens && count > 0)
        {
            for (int i = 0; i < count; ++i)
            {
                screenRects.append(QRect(screens[i].x_org, screens[i].y_org, screens[i].width, screens[i].height));
            }
            XFree(screens);
        }
    }

    // 索引越界返回无效值
    if (screenIdx >= screenRects.size())
        return QRect(-1, -1, -1, -1);
    return screenRects.at(screenIdx);
}

// ======================================================================================
// ========== 核心新增：多屏工具函数 2 → 本地坐标 转 全局坐标 核心实现 ==========
// ======================================================================================
void MouseSimulator::localToGlobal(int screenIdx, int &localX, int &localY, int &globalX, int &globalY) const
{
    globalX          = -1;
    globalY          = -1;
    QRect screenRect = getScreenRectByIndex(screenIdx);
    if (screenRect.isValid() == false)
    {
        LOG_MESSAGE("MouseSimulator", QString("屏幕索引[%1]无效，无法转换坐标").arg(screenIdx));
        return;
    }
    // 核心公式：全局坐标 = 屏幕偏移量 + 本地坐标
    globalX = screenRect.x() + localX;
    globalY = screenRect.y() + localY;

    // 边界校验：坐标不能超出屏幕范围
    if (globalX > screenRect.right())
        globalX = screenRect.right();
    if (globalY > screenRect.bottom())
        globalY = screenRect.bottom();
    if (globalX < screenRect.x())
        globalX = screenRect.x();
    if (globalY < screenRect.y())
        globalY = screenRect.y();
}
