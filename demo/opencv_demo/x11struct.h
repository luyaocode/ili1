#ifndef X11STRUCT_H
#define X11STRUCT_H
#include <X11/Xlib.h>
#include <X11/Xutil.h>
struct x11struct
{
    Display* m_display;
    Window m_rootWindow;
    x11struct(Display* display,Window window):m_display(display),m_rootWindow(window){}
};

#endif // X11STRUCT_H
