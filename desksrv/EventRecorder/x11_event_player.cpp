#ifndef MOUSESIMULATOR_H
#define MOUSESIMULATOR_H

#include <QObject>
#include <QString>
#include <X11/Xlib.h>

// 手动声明XTest核心函数（替代XTest.h，兼容缺少头文件场景）
extern "C" {
// XTest扩展可用性检查
Bool XTestQueryExtension(Display *display, int *event_base_return, int *error_base_return);

// 模拟鼠标按键事件（兼容void*和Display*参数类型）
void XTestFakeButtonEvent(
    void* display,
    unsigned int button,
    int press,
    unsigned long delay
);

// 模拟鼠标移动事件
void XTestFakeMotionEvent(
    Display* display,
    int deviceid,
    int x,
    int y,
    unsigned long delay
);

// 模拟键盘事件（预留，当前未使用）
void XTestFakeKeyEvent(Display *dpy, unsigned int keycode, Bool press, unsigned long delay);
}

// 鼠标事件模拟工具类（X11环境）
// 支持：鼠标移动、左右中键点击/双击、滚轮滚动
class MouseSimulator : public QObject
{
    Q_OBJECT
public:
    // 鼠标按键枚举（简化调用）
    enum MouseButton {
        LeftButton = Button1,    // 左键
        RightButton = Button3,   // 右键（X11中Button2是中键，Button3是右键）
        MiddleButton = Button2   // 中键
    };
    Q_ENUM(MouseButton)

    // 滚轮方向枚举
    enum WheelDirection {
        WheelUp,    // 向上滚动
        WheelDown,  // 向下滚动
        WheelLeft,  // 向左滚动
        WheelRight  // 向右滚动
    };
    Q_ENUM(WheelDirection)

    // 单例模式（全局唯一实例，避免重复连接X11）
    static MouseSimulator* getInstance(QObject *parent = nullptr);

    // ========== 核心接口 ==========
    /**
     * @brief 移动鼠标到指定绝对坐标
     * @param x 目标X坐标（X11原点在左上角）
     * @param y 目标Y坐标
     * @return 成功返回true，失败返回false
     */
    bool moveMouse(int x, int y);

    /**
     * @brief 相对当前位置移动鼠标
     * @param dx X轴偏移量（正数右移，负数左移）
     * @param dy Y轴偏移量（正数下移，负数上移）
     * @return 成功返回true，失败返回false
     */
    bool moveMouseRelative(int dx, int dy);

    /**
     * @brief 触发鼠标按键点击（按下+释放）
     * @param button 鼠标按键（Left/Right/Middle）
     * @param x 点击位置X坐标（-1表示使用当前鼠标位置）
     * @param y 点击位置Y坐标（-1表示使用当前鼠标位置）
     * @return 成功返回true，失败返回false
     */
    bool clickMouse(MouseButton button, int x = -1, int y = -1);

    /**
     * @brief 触发鼠标按键双击
     * @param button 鼠标按键（Left/Right/Middle）
     * @param x 双击位置X坐标（-1表示使用当前鼠标位置）
     * @param y 双击位置Y坐标（-1表示使用当前鼠标位置）
     * @param interval 双击间隔（毫秒，默认500ms）
     * @return 成功返回true，失败返回false
     */
    bool doubleClickMouse(MouseButton button, int x = -1, int y = -1, int interval = 500);

    /**
     * @brief 单独触发鼠标按键按下
     * @param button 鼠标按键（Left/Right/Middle）
     * @return 成功返回true，失败返回false
     */
    bool pressMouse(MouseButton button);

    /**
     * @brief 单独触发鼠标按键释放
     * @param button 鼠标按键（Left/Right/Middle）
     * @return 成功返回true，失败返回false
     */
    bool releaseMouse(MouseButton button);

    /**
     * @brief 鼠标滚轮滚动
     * @param direction 滚动方向（Up/Down/Left/Right）
     * @param steps 滚动步数（默认1步，步数越多滚动距离越长）
     * @return 成功返回true，失败返回false
     */
    bool scrollWheel(WheelDirection direction, int steps = 1);

    /**
     * @brief 获取当前鼠标位置
     * @param x 输出当前X坐标
     * @param y 输出当前Y坐标
     * @return 成功返回true，失败返回false
     */
    bool getCurrentMousePos(int &x, int &y);

private:
    explicit MouseSimulator(QObject *parent = nullptr);
    ~MouseSimulator() override;

    // 禁止拷贝
    MouseSimulator(const MouseSimulator&) = delete;
    MouseSimulator& operator=(const MouseSimulator&) = delete;

    Display *m_display; // X11显示连接（核心句柄）
    static MouseSimulator *m_instance; // 单例实例
};

#endif // MOUSESIMULATOR_H
