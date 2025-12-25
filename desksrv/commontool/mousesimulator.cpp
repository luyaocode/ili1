#include "mousesimulator.h"
#include <QThread>
#include <QDebug>
#include "virtualmousewidget.h"
#include "x11struct.h"

// 静态单例初始化
MouseSimulator *MouseSimulator::m_instance = nullptr;

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
    if (!m_x11struct->m_x11Display)
        return false;

    // XTest模拟绝对移动（0号鼠标设备）
    XTestFakeMotionEvent(m_x11struct->m_x11Display, 0, x, y, CurrentTime);
    XFlush(m_x11struct->m_x11Display);  // 立即刷新事件队列，确保生效
                                        //    // 同步更新虚拟鼠标绘制位置
                                        //       if (m_virtualMouseWidget) {
                                        //           m_virtualMouseWidget->updateMousePos(x, y);
                                        //       }
    qDebug() << "[MouseSimulator] 鼠标移动到绝对坐标：" << x << "," << y;
    return true;
}

bool MouseSimulator::moveMouseRelative(int dx, int dy)
{
    if (!m_x11struct->m_x11Display)
        return false;

    // 先获取当前位置，再计算相对偏移
    int curX, curY;
    if (!getCurrentMousePos(curX, curY))
    {
        qWarning() << "[MouseSimulator] 获取当前鼠标位置失败，无法相对移动";
        return false;
    }

    int  newX = curX + dx;
    int  newY = curY + dy;
    bool ret  = moveMouse(newX, newY);  // 复用绝对移动（自动同步绘制）
    return ret;
}

bool MouseSimulator::clickMouse(MouseButton button, int x, int y)
{
    //    if (!m_x11struct->m_x11Display)
    //        return false;

    //    // 如果指定了坐标，先移动到目标位置
    //    if (x >= 0 && y >= 0)
    //    {
    //        moveMouse(x, y);
    //    }

    //    // 按下 + 释放 = 单次点击（适配void*参数类型）
    //    XTestFakeButtonEvent((void *)m_x11struct->m_x11Display, button, True, CurrentTime);   // 按下
    //    XTestFakeButtonEvent((void *)m_x11struct->m_x11Display, button, False, CurrentTime);  // 释放
    //    XFlush(m_x11struct->m_x11Display);

    //    qDebug() << "[MouseSimulator] 鼠标" << (button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
    //             << "点击，坐标：" << (x >= 0 ? QString::number(x) : "当前位置") << ","
    //             << (y >= 0 ? QString::number(y) : "当前位置");
    //    return true;

    // 先按下
    if (!pressMouse(button, x, y))
    {
        return false;
    }
    // 可选：添加短延迟（模拟真实点击的按下-释放间隔）
    // usleep(50 * 1000); // 50ms延迟
    // 再释放
    return releaseMouse(button, x, y);
}

bool MouseSimulator::pressMouse(MouseSimulator::MouseButton button, int x, int y)
{
    if (button == MouseSimulator::MouseButton::ButtonNone)
    {
        return false;
    }
    // 校验X11连接有效性
    if (!m_x11struct || !m_x11struct->m_x11Display)
    {
        qWarning() << "[MouseSimulator] X11显示连接无效，无法按下鼠标";
        return false;
    }

    // 如果指定了坐标，先移动到目标位置
    if (x >= 0 && y >= 0)
    {
        if (!moveMouse(x, y))
        {
            qWarning() << "[MouseSimulator] 移动鼠标到坐标(" << x << "," << y << ")失败";
            return false;
        }
    }

    // 仅发送鼠标按下事件（不释放，实现长按）
    XTestFakeButtonEvent(m_x11struct->m_x11Display, button, True, CurrentTime);
    XFlush(m_x11struct->m_x11Display);

    // 日志输出
    qDebug() << "[MouseSimulator] 鼠标" << (button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
             << "按下，坐标：" << (x >= 0 ? QString::number(x) : "当前位置") << ","
             << (y >= 0 ? QString::number(y) : "当前位置");
    return true;
}

bool MouseSimulator::releaseMouse(MouseSimulator::MouseButton button, int x, int y)
{
    if (button == MouseSimulator::MouseButton::ButtonNone)
    {
        return false;
    }
    // 校验X11连接有效性
    if (!m_x11struct || !m_x11struct->m_x11Display)
    {
        qWarning() << "[MouseSimulator] X11显示连接无效，无法释放鼠标";
        return false;
    }

    // 可选：若指定坐标，先移动到目标位置（保证释放位置和按下位置一致）
    if (x >= 0 && y >= 0)
    {
        moveMouse(x, y);  // 即使移动失败，仍尝试释放按键（避免按键卡死）
    }

    // 仅发送鼠标释放事件
    XTestFakeButtonEvent(m_x11struct->m_x11Display, button, False, CurrentTime);
    XFlush(m_x11struct->m_x11Display);

    // 日志输出
    qDebug() << "[MouseSimulator] 鼠标" << (button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
             << "释放，坐标：" << (x >= 0 ? QString::number(x) : "当前位置") << ","
             << (y >= 0 ? QString::number(y) : "当前位置");
    return true;
}

bool MouseSimulator::doubleClickMouse(MouseButton button, int x, int y, int interval)
{
    if (!m_x11struct->m_x11Display)
        return false;

    // 第一次点击
    clickMouse(button, x, y);
    // 双击间隔（模拟人类操作延迟）
    QThread::msleep(interval);
    // 第二次点击
    clickMouse(button, x, y);

    qDebug() << "[MouseSimulator] 鼠标" << (button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
             << "双击，坐标：" << (x >= 0 ? QString::number(x) : "当前位置") << ","
             << (y >= 0 ? QString::number(y) : "当前位置");
    return true;
}

bool MouseSimulator::pressMouse(MouseButton button)
{
    if (!m_x11struct->m_x11Display)
        return false;

    XTestFakeButtonEvent((void *)m_x11struct->m_x11Display, button, True, CurrentTime);
    XFlush(m_x11struct->m_x11Display);
    //    // 同步更新虚拟鼠标按压状态
    //      if (m_virtualMouseWidget) {
    //          m_virtualMouseWidget->updateMousePressState(true);
    //      }

    qDebug() << "[MouseSimulator] 鼠标" << (button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
             << "按下";
    return true;
}

bool MouseSimulator::releaseMouse(MouseButton button)
{
    if (!m_x11struct->m_x11Display)
        return false;

    XTestFakeButtonEvent((void *)m_x11struct->m_x11Display, button, False, CurrentTime);
    XFlush(m_x11struct->m_x11Display);
    //    // 同步更新虚拟鼠标释放状态
    //    if (m_virtualMouseWidget) {
    //        m_virtualMouseWidget->updateMousePressState(false);
    //    }
    qDebug() << "[MouseSimulator] 鼠标" << (button == LeftButton ? "左键" : (button == RightButton ? "右键" : "中键"))
             << "释放";
    return true;
}

bool MouseSimulator::scrollWheel(WheelDirection direction, int steps)
{
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

    qDebug() << "[MouseSimulator] 鼠标滚轮"
             << (direction == WheelUp ? "上" : (direction == WheelDown ? "下" : (direction == WheelLeft ? "左" : "右")))
             << "滚动" << steps << "步";
    return true;
}

bool MouseSimulator::getCurrentMousePos(int &x, int &y)
{
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

    qWarning() << "[MouseSimulator] 获取当前鼠标位置失败";
    return false;
}
