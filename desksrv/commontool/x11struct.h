#ifndef X11STRUCT_H
#define X11STRUCT_H
// X11 相关头文件
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
struct x11struct
{
    Display *m_x11Display;       // X Server 连接句柄
    Window   m_clipboardWindow;  //
    x11struct()
    {
        m_x11Display      = nullptr;
        m_clipboardWindow = None;
    }
};
#define SHIFT_MASK "ShiftMask"
#define LOCK_MASK "LockMask"
#define CONTROL_MASK "ControlMask"
#define MOD1_MASK "Mod1Mask"
#define MOD2_MASK "Mod2Mask"
#define MOD3_MASK "Mod3Mask"
#define MOD4_MASK "Mod4Mask"
#define MOD5_MASK "Mod5Mask"



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

#endif  // X11STRUCT_H
