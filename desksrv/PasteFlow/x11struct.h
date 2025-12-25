#ifndef X11STRUCT_H
#define X11STRUCT_H
// X11 相关头文件
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
struct x11struct
{
    Display *m_x11Display; // X Server 连接句柄
    Window m_clipboardWindow; //
    x11struct()
    {
        m_x11Display = nullptr;
        m_clipboardWindow = None;
    }
};
#endif // X11STRUCT_H
