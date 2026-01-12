#ifndef MOUSESIMULATOR_H
#define MOUSESIMULATOR_H

#include <QObject>
#include <QString>
#include <QMutex>

//#include "X11/X.h"
struct x11struct;
//class VirtualMouseWidget;
#ifndef Button6
#    define Button6 6
#endif
#ifndef Button7
#    define Button7 7
#endif
// 鼠标事件模拟工具类（X11环境）
// 支持：鼠标移动、左右中键点击/双击、滚轮滚动
class MouseSimulator : public QObject
{
    Q_OBJECT
public:
    // 鼠标按键枚举（简化调用）
    enum MouseButton
    {
        ButtonNone   = 0,
        LeftButton   = 1,  // 左键
        MiddleButton = 2,  // 中键
        RightButton  = 3,  // 右键（X11中Button2是中键，Button3是右键）
    };
    Q_ENUM(MouseButton)

    // 滚轮方向枚举
    enum WheelDirection
    {
        WheelNone,
        WheelUp,    // 向上滚动
        WheelDown,  // 向下滚动
        WheelLeft,  // 向左滚动
        WheelRight  // 向右滚动
    };
    Q_ENUM(WheelDirection)

    // 单例模式（全局唯一实例，避免重复连接X11）
    static MouseSimulator *getInstance();
    static void            deleteInstance();

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
     * 鼠标按下（长按）函数
     * @param button 鼠标按键（左/右/中键）
     * @param x 目标X坐标（-1表示不移动，使用当前位置）
     * @param y 目标Y坐标（-1表示不移动，使用当前位置）
     * @return 按下是否成功
     */
    bool pressMouse(MouseButton button, int x = -1, int y = -1);

    /**
     * 鼠标释放函数（需与pressMouse配对使用）
     * @param button 鼠标按键（必须和pressMouse的按键一致）
     * @param x 目标X坐标（-1表示不移动，使用当前位置；建议和pressMouse坐标一致）
     * @param y 目标Y坐标（-1表示不移动，使用当前位置；建议和pressMouse坐标一致）
     * @return 释放是否成功
     */
    bool releaseMouse(MouseButton button, int x = -1, int y = -1);

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

    // 新增：鼠标按钮枚举保持不变，新增【多屏核心函数】
    bool moveMouse(int screenIdx, int localX, int localY);             // 根据屏幕索引+本地坐标移动
    bool clickMouse(MouseButton button, int screenIdx, int x, int y);  // 根据屏幕索引+本地坐标点击
    bool pressMouse(MouseButton button, int screenIdx, int x, int y);
    bool releaseMouse(MouseButton button, int screenIdx, int x, int y);
    bool doubleClickMouse(MouseButton button, int screenIdx, int x, int y, int interval = 200);

private:
    explicit MouseSimulator(QObject *parent = nullptr);
    ~MouseSimulator() override;

    // 禁止拷贝
    MouseSimulator(const MouseSimulator &)            = delete;
    MouseSimulator &operator=(const MouseSimulator &) = delete;

    // 新增：多屏核心工具函数 - 根据屏幕索引，获取该屏幕的【全局偏移+分辨率】，返回QRect(x偏移,y偏移,宽,高)
    QRect getScreenRectByIndex(int screenIdx) const;
    // 新增：多屏核心工具函数 - 本地坐标 → 全局坐标 转换
    void localToGlobal(int screenIdx, int &localX, int &localY, int &globalX, int &globalY) const;

private:
    x11struct             *m_x11struct;  // X11显示连接（核心句柄）
    static MouseSimulator *m_instance;   // 单例实例
    static QMutex          m_mutex;
    //    VirtualMouseWidget    *m_virtualMouseWidget;  // 新增：虚拟鼠标绘制窗口
};

#endif  // MOUSESIMULATOR_H
