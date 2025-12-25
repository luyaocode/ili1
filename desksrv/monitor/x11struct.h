#ifndef X11STRUCT_H
#define X11STRUCT_H
// X11 相关头文件
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
struct x11struct
{
    Display *m_x11Display; // X Server 连接句柄
    Window m_x11RootWindow; // 系统根窗口
    Atom m_wmDeleteMessage;  // 窗口关闭消息
    XEvent* event;
    x11struct()
    {
        m_x11Display = nullptr;
        m_wmDeleteMessage = None;
        event = nullptr;
    }
};
#endif // X11STRUCT_H
