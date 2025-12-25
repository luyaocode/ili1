#include "globaltool.h"
//#include <QStringList>
// 直接包含系统已有的 XTest 协议头文件
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/X.h>       // 基础类型（Bool、Window 等）
#include <X11/Xproto.h>  // 协议类型（CARD8、CARD16、BYTE、INT16 等）
#include <X11/extensions/xtestproto.h>
#include <X11/extensions/xtestext1.h>
#include <X11/Xatom.h>  // XA_PRIMARY 定义在此文件中
#include <X11/Xutil.h>  // 关键：XClassHint 结构体定义在此
#include <unistd.h>
#include <stdio.h>
#include <algorithm>
#include <sstream>
#include <string.h>  // strcmp 所需（可选，部分系统默认包含）
// 用于字符串转数值的函数
#include <cstdlib>
// 用于错误处理
#include <cerrno>
#include <iostream>
extern "C"
{
#include <fcntl.h>
#include <signal.h> /* 信号定义 */
#include <stdarg.h> /* 可变参数定义 */
#include <stdio.h>  /* 标准输入输出定义 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> /* Unix 标准函数定义 */
}

// 手动声明 XTest 核心函数（因缺少 XTest.h，直接声明函数原型）
extern "C"
{
    void XTestFakeKeyEvent(Display *dpy, unsigned int keycode, Bool press, unsigned long delay);
}

///////////////////////////////////// 辅助函数////////////////////////////////////////////
///// 辅助函数：获取窗口的顶层父窗口（跳过子窗口/弹窗，找到应用主窗口）
Window getTopLevelWindow(Display *display, Window window)
{
    Window       rootWindow, parentWindow, *childWindows;
    unsigned int numChildren;

    // 循环向上查找，直到父窗口是根窗口（顶层窗口的父窗口是根窗口）
    while (1)
    {
        if (!XQueryTree(display, window, &rootWindow, &parentWindow, &childWindows, &numChildren))
        {
            fprintf(stderr, "警告：无法查询窗口树，使用当前窗口作为顶层窗口\n");
            return window;
        }

        // 释放子窗口列表（XQueryTree 分配的内存）
        if (childWindows)
            XFree(childWindows);

        // 父窗口是根窗口 → 当前窗口是顶层窗口
        if (parentWindow == rootWindow || parentWindow == None)
        {
            return window;
        }

        // 否则继续向上查找父窗口
        window = parentWindow;
    }
}

// 辅助函数：获取窗口的 WM_CLASS（res_class）
char *getWindowClass(Display *display, Window window)
{
    Atom           wmClassAtom = XInternAtom(display, "WM_CLASS", True);
    Atom           actualType;
    int            actualFormat;
    unsigned long  itemCount, bytesAfter;
    unsigned char *propValue = NULL;

    // 获取 WM_CLASS 属性
    int status = XGetWindowProperty(display, window, wmClassAtom, 0, 1024, False, AnyPropertyType, &actualType,
                                    &actualFormat, &itemCount, &bytesAfter, &propValue);

    if (status != Success || propValue == NULL)
    {
        return NULL;
    }

    // 解析 res_class（WM_CLASS 的第二个字符串）
    char *resName  = (char *)propValue;
    char *resClass = resName + strlen(resName) + 1;
    if (strlen(resClass) == 0)
    {
        XFree(propValue);
        return NULL;
    }

    // 复制 res_class 到新内存（避免后续释放 propValue 导致失效）
    char *className = strdup(resClass);
    XFree(propValue);
    return className;
}

// 辅助函数：查找可接受键盘输入的子窗口
Window find_input_subwindow(Display *display, Window root)
{
    Window       parent, *children;
    unsigned int nchildren;
    if (XQueryTree(display, root, &root, &parent, &children, &nchildren))
    {
        for (unsigned int i = 0; i < nchildren; i++)
        {
            // 检查子窗口是否可接受键盘输入（简化判断，实际可更严格）
            XWindowAttributes attr;
            if (XGetWindowAttributes(display, children[i], &attr) && attr.map_state == IsViewable &&  // 窗口可见
                (attr.all_event_masks & KeyPressMask))
            {  // 监听按键事件
                XFree(children);
                return children[i];
            }
            // 递归查找更深层的子窗口
            Window sub = find_input_subwindow(display, children[i]);
            if (sub != None)
            {
                XFree(children);
                return sub;
            }
        }
        XFree(children);
    }
    return root;  // 找不到子窗口则返回自身
}

// 获取焦点窗口
Window getFocusedWindow()
{
    // 1. 打开 X11 显示连接
    const char *displayEnv = getenv("DISPLAY");
    Display    *display    = XOpenDisplay(displayEnv ? displayEnv : NULL);
    if (!display)
    {
        fprintf(stderr, "错误：无法打开 X11 显示连接（DISPLAY=%s）\n", displayEnv ? displayEnv : "未设置");
        return false;
    }

    // 2. 获取焦点窗口 → 查找顶层父窗口（关键修复：跳过弹窗/子窗口）
    Window focusWindow, topWindow;
    int    revertTo;
    XGetInputFocus(display, &focusWindow, &revertTo);

    if (focusWindow == None || focusWindow == BadWindow)
    {
        fprintf(stderr, "错误：未找到有效焦点窗口\n");
        XCloseDisplay(display);
        return false;
    }

    // 找到焦点窗口的顶层父窗口（应用主窗口）
    topWindow = getTopLevelWindow(display, focusWindow);
    if (topWindow == None)
    {
        fprintf(stderr, "错误：无法找到顶层窗口\n");
        XCloseDisplay(display);
        return false;
    }
    return topWindow;
}

bool isAlreadyRunning(const std::string &name)
{
    std::string  lock_file  = "/tmp/" + name + ".pid";
    const char  *LOCK_FILE  = lock_file.c_str();
    int          fdLockFile = -1;
    char         szPid[32];
    struct flock fl;

    /* 打开锁文件 */
    fdLockFile = open(LOCK_FILE, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fdLockFile < 0)
    {
        fprintf(stderr, "lock file open failed");
        exit(EXIT_FAILURE);
    }

    /* 对整个锁文件加写锁 */
    fl.l_type   = F_WRLCK;   //记录锁类型：独占性写锁
    fl.l_whence = SEEK_SET;  //加锁区域起点:距文件开始处l_start个字节
    fl.l_start  = 0;
    fl.l_len =
        0;  //加锁区域终点:0表示加锁区域自起点开始直至文件最大可能偏移量为止,不管写入多少字节在文件末尾,都属于加锁范围
    if (fcntl(fdLockFile, F_SETLK, &fl) < 0)
    {
        if (EACCES == errno || EAGAIN == errno)  //系统中已有该守护进程的实例在运行
        {
            fprintf(stdout, "%s Already Running...", name.c_str());
            close(fdLockFile);
            return true;
        }

        fprintf(stdout, "%s AlreadyRunning fcntl", name.c_str());

        exit(EXIT_FAILURE);
    }

    /* 清空锁文件,然后将当前守护进程pid写入锁文件 */
    ftruncate(fdLockFile, 0);
    sprintf(szPid, "%ld", (long)getpid());
    write(fdLockFile, szPid, strlen(szPid) + 1);

    return false;
}

// 新增：检查是否运行在 Wayland 环境
bool isWayland()
{
    const char *waylandDisplay = getenv("WAYLAND_DISPLAY");
    return waylandDisplay != nullptr && strlen(waylandDisplay) > 0;
}

// 激活 X11 的 PRIMARY 选择器（通知应用数据已更新）
void activateX11PrimarySelection(const QMimeData *mimeData)
{
    Display *display = XOpenDisplay(NULL);
    if (!display || !mimeData)
        return;

    // 获取 PRIMARY 选择器原子
    Atom primaryAtom = XA_PRIMARY;
    Atom targetsAtom = XInternAtom(display, "TARGETS", False);
    //    Atom textAtom = XInternAtom(display, "UTF8_STRING", False);

    // 发送选择器变化事件（通知所有监听 PRIMARY 的应用）
    XSetSelectionOwner(display, primaryAtom, DefaultRootWindow(display), CurrentTime);
    XEvent event;
    event.xselection.type      = SelectionNotify;
    event.xselection.display   = display;
    event.xselection.requestor = DefaultRootWindow(display);
    event.xselection.selection = primaryAtom;
    event.xselection.target    = targetsAtom;
    event.xselection.time      = CurrentTime;
    XSendEvent(display, DefaultRootWindow(display), False, NoEventMask, &event);
    XSync(display, False);
    XCloseDisplay(display);
}

int send_x11_key_combination(unsigned int mod_mask, KeySym keysym, int delay_ms)
{
    // Wayland 环境下 X11 接口失效，直接返回错误
    if (isWayland())
    {
        fprintf(stderr, "错误：当前是 Wayland 环境，X11 按键模拟不支持（需使用 wtype 工具）\n");
        return -10;
    }

    // 1. 参数校验
    if (delay_ms < 50)
        delay_ms = 50;  // 延长延时到 50ms，确保应用响应
    if (keysym == NoSymbol)
    {
        fprintf(stderr, "错误：无效的 X11 键符号\n");
        return -1;
    }

    // 2. 打开 X11 显示连接
    Display *display = XOpenDisplay(NULL);
    if (!display)
    {
        fprintf(stderr, "错误：无法连接到 X11 显示（DISPLAY 环境变量未设置？）\n");
        return -2;
    }

    // 3. 键符号转系统键码（兼容不同键盘布局）
    KeyCode keycode = XKeysymToKeycode(display, keysym);
    if (keycode == 0)
    {
        fprintf(stderr, "错误：X11 键符号 %lu 无法转换为键码\n", (unsigned long)keysym);
        XCloseDisplay(display);
        return -3;
    }

    // 4. 获取当前焦点窗口
    Window focusWindow, targetWindow;
    int    revertTo;
    XGetInputFocus(display, &focusWindow, &revertTo);
    targetWindow = focusWindow;

    // 5. 辅助函数：发送 X11 按键事件（优化：使用 XSendEvent + XSync）
    auto send_x11_event = [&](KeyCode kc, bool is_press) {
        XKeyEvent event;
        event.display     = display;
        event.window      = targetWindow;  // 发送到目标
        event.root        = DefaultRootWindow(display);
        event.subwindow   = None;
        event.time        = CurrentTime;
        event.x           = 0;
        event.y           = 0;
        event.x_root      = 0;
        event.y_root      = 0;
        event.same_screen = True;
        event.keycode     = kc;
        event.state       = mod_mask;  // 关键：设置事件的 state 为修饰键掩码
        event.type        = is_press ? KeyPress : KeyRelease;

        // 发送事件并同步（XSync 确保事件被处理）
        XSendEvent(display, targetWindow, False, KeyPressMask | KeyReleaseMask, (XEvent *)&event);
        XSync(display, False);  // 比 XFlush 更可靠，确保事件已传递到窗口
    };

    // 6. 修饰键映射（保持不变）
    struct ModKeyMap
    {
        unsigned int mask;
        KeySym       keysym;
    } mod_key_maps[] = {{ControlMask, XK_Control_L},
                        {ShiftMask, XK_Shift_L},
                        {Mod1Mask, XK_Alt_L},
                        {Mod4Mask, XK_Super_L},
                        {0, NoSymbol}};

    // 7. 模拟按键流程（优化延时）
    // 按下修饰键
    for (ModKeyMap *map = mod_key_maps; map->mask != 0; map++)
    {
        if (mod_mask & map->mask)
        {
            KeyCode mod_keycode = XKeysymToKeycode(display, map->keysym);
            if (mod_keycode != 0)
            {
                send_x11_event(mod_keycode, true);
                usleep(delay_ms * 1000);
            }
        }
    }

    // 按下目标键
    send_x11_event(keycode, true);
    usleep(delay_ms * 1000);

    // 释放目标键
    send_x11_event(keycode, false);
    usleep(delay_ms * 1000);

    // 释放修饰键（逆序）
    for (int i = sizeof(mod_key_maps) / sizeof(ModKeyMap) - 2; i >= 0; i--)
    {
        ModKeyMap *map = &mod_key_maps[i];
        if (mod_mask & map->mask)
        {
            KeyCode mod_keycode = XKeysymToKeycode(display, map->keysym);
            if (mod_keycode != 0)
            {
                send_x11_event(mod_keycode, false);
                usleep(delay_ms * 1000);
            }
        }
    }

    // 8. 关闭连接
    XCloseDisplay(display);
    printf("成功发送 X11 组合键：修饰键掩码=0x%x，键符号=0x%lu，目标窗口=0x%lx\n", mod_mask, (unsigned long)keysym,
           (unsigned long)targetWindow);
    return 0;
}

bool isTerminalWindowX11()
{
    bool isTerminal = false;
    // 1. 打开 X11 显示连接
    const char *displayEnv = getenv("DISPLAY");
    Display    *display    = XOpenDisplay(displayEnv ? displayEnv : NULL);
    if (!display)
    {
        fprintf(stderr, "错误：无法打开 X11 显示连接（DISPLAY=%s）\n", displayEnv ? displayEnv : "未设置");
        return false;
    }

    // 2. 获取焦点窗口 → 查找顶层父窗口（关键修复：跳过弹窗/子窗口）
    Window focusWindow, topWindow;
    int    revertTo;
    XGetInputFocus(display, &focusWindow, &revertTo);

    if (focusWindow == None || focusWindow == BadWindow)
    {
        fprintf(stderr, "错误：未找到有效焦点窗口\n");
        XCloseDisplay(display);
        return false;
    }

    // 找到焦点窗口的顶层父窗口（应用主窗口）
    topWindow = getTopLevelWindow(display, focusWindow);
    if (topWindow == None)
    {
        fprintf(stderr, "错误：无法找到顶层窗口\n");
        XCloseDisplay(display);
        return false;
    }

    // 3. 获取顶层窗口的类名
    char *className = getWindowClass(display, topWindow);
    if (className)
    {
        // 4. 扩展终端类名列表（覆盖更多场景）
        const char *terminalClasses[] = {"gnome-terminal-server",
                                         "konsole",
                                         "XTerm",
                                         "kitty",
                                         "Alacritty",
                                         "lxterminal",
                                         "xfce4-terminal",
                                         "terminator",
                                         "tilix",
                                         "st",
                                         "urxvt",
                                         "rxvt",
                                         "termite",
                                         "foot",
                                         "wezterm",
                                         "deepin-terminal",
                                         "mate-terminal",
                                         "qterminal",
                                         "gnome-terminal",
                                         "konsole-2",
                                         "urxvtc",
                                         "urxvtd",
                                         NULL};

        // 5. 类名匹配（支持可选忽略大小写）
        for (int i = 0; terminalClasses[i] != NULL; i++)
        {
            // 如需忽略大小写，将 strcmp 改为 strcasecmp（需包含 <strings.h>）
            if (strcasecmp(className, terminalClasses[i]) == 0)
            {
                isTerminal = true;
                break;
            }
        }
    }
    else
    {
        fprintf(stderr, "警告：顶层窗口无有效 WM_CLASS 属性（非标准应用窗口）\n");
    }
    // 6. 输出关键信息（方便调试）
    fprintf(stdout, "X11 窗口信息：\n");
    fprintf(stdout, "  焦点窗口ID：0x%lx\n", focusWindow);
    fprintf(stdout, "  顶层窗口ID：0x%lx\n", topWindow);
    fprintf(stdout, "  顶层窗口类名：%s\n", className ? className : "unknown");
    fprintf(stdout, "  是否为终端：%s\n", isTerminal ? "是" : "否");

    // 7. 释放资源
    if (className)
        free(className);  // 释放 strdup 分配的内存
    XCloseDisplay(display);

    return isTerminal;
}

bool simulateKeyWithMask(const std::vector<KeySym> &maskKeys, KeySym targetKey, int delayMs)
{
    //    // 打开X服务器连接
    //    Display *display = XOpenDisplay(nullptr);
    //    if (!display)
    //    {
    //        fprintf(stderr, "无法连接到X服务器（检查DISPLAY环境变量）\n");
    //        return false;
    //    }

    //    // 转换延迟为微秒
    //    const int delayUs = delayMs * 1000;

    //    // 1. 验证所有按键的有效性（转换为键码）
    //    std::vector<KeyCode> maskKeyCodes;
    //    for (KeySym maskSym : maskKeys)
    //    {
    //        KeyCode code = XKeysymToKeycode(display, maskSym);
    //        if (code == 0)
    //        {
    //            fprintf(stderr, "无效的掩码键（Keysym：%u）\n", (unsigned int)maskSym);
    //            XCloseDisplay(display);
    //            return false;
    //        }
    //        maskKeyCodes.push_back(code);
    //    }

    //    KeyCode targetCode = XKeysymToKeycode(display, targetKey);
    //    if (targetCode == 0)
    //    {
    //        fprintf(stderr, "无效的目标键（Keysym：%u）\n", (unsigned int)targetKey);
    //        XCloseDisplay(display);
    //        return false;
    //    }

    //    // 打印信息
    //    std::string info;
    //    for (auto mask : maskKeys)
    //    {
    //        info += keysymToString(mask) + " + ";
    //    }
    //    info += keysymToString(targetKey);
    //    fprintf(stdout, "模拟按键：%s\n", info.c_str());

    //    // 2. 模拟按键序列：按下所有掩码键 → 按下目标键 → 释放目标键 → 释放所有掩码键
    //    // 按下掩码键（如Ctrl、Shift）
    //    for (KeyCode code : maskKeyCodes)
    //    {
    //        XTestFakeKeyEvent(display, code, True, 0);  // 按下掩码键
    //        XFlush(display);
    //        usleep(delayUs);
    //    }

    //    // 按下并释放目标键
    //    XTestFakeKeyEvent(display, targetCode, True, 0);  // 按下目标键
    //    XFlush(display);
    //    usleep(delayUs);

    //    XTestFakeKeyEvent(display, targetCode, False, 0);  // 释放目标键
    //    XFlush(display);
    //    usleep(delayUs);

    //    // 释放掩码键（与按下顺序相反，避免冲突）
    //    for (auto it = maskKeyCodes.rbegin(); it != maskKeyCodes.rend(); ++it)
    //    {
    //        XTestFakeKeyEvent(display, *it, False, 0);  // 释放掩码键
    //        XFlush(display);
    //        usleep(delayUs);
    //    }

    //    // 关闭连接
    //    XCloseDisplay(display);
    //    return true;
    // 先按下
    if (!simulatePressKeyWithMask(maskKeys, targetKey, delayMs))
    {
        return false;
    }
    // 再释放
    return simulateReleaseKeyWithMask(maskKeys, targetKey, delayMs);
}

std::string modifierToString(unsigned int modifier)
{
    std::vector<std::string> mods;

    // 按常用优先级判断修饰码
    if (modifier & ControlMask)
        mods.push_back("Ctrl");
    if (modifier & ShiftMask)
        mods.push_back("Shift");
    if (modifier & Mod1Mask)
        mods.push_back("Alt");  // Mod1Mask 对应 Alt
    if (modifier & Mod4Mask)
        mods.push_back("Meta");  // Mod4Mask 对应 Windows/Super 键
    if (modifier & Mod3Mask)
        mods.push_back("AltGr");  // 可选：右侧Alt
    if (modifier & LockMask)
        mods.push_back("CapsLock");
    if (modifier & Mod2Mask)
        mods.push_back("NumLock");

    // 拼接修饰码（如 "Ctrl+Shift"）
    std::ostringstream oss;
    for (size_t i = 0; i < mods.size(); ++i)
    {
        if (i > 0)
            oss << "+";
        oss << mods[i];
    }
    return oss.str();
}

std::string keysymToString(KeySym keysym)
{
    // XKeysymToString 是X11内置函数，直接转换大部分键码
    const char *keyStr = XKeysymToString(keysym);
    if (keyStr)
    {
        // 对部分特殊键码做友好化处理（可选）
        std::string res = keyStr;
        // 示例：将 "Return" 转为 "Enter"，"backslash" 转为 "\" 等
        if (res == "Return")
            return "Enter";
        if (res == "backslash")
            return "\\";
        if (res == "space")
            return "Space";
        if (res == "Delete")
            return "Del";
        if (res == "Prior")
            return "PageUp";
        if (res == "Next")
            return "PageDown";
        if (res == "Escape")
            return "Esc";
        return res;
    }

    // 处理特殊/未知键码
    return "Unknown";
}

std::string keyCombinationToString(unsigned int modifier, KeySym keysym)
{
    std::string modStr = modifierToString(modifier);
    std::string keyStr = keysymToString(keysym);

    if (modStr.empty())
    {
        return keyStr;
    }
    else
    {
        return modStr + "+" + keyStr;
    }
}

std::string getKeyEventString(XEvent &event)
{
    std::ostringstream oss;

    // 非按键事件返回提示
    if (event.type != KeyPress && event.type != KeyRelease)
    {
        oss << "非按键事件";
        return oss.str();
    }

    // 提取核心数据
    XKeyEvent   &keyEvent = event.xkey;
    unsigned int modifier = keyEvent.state;
    KeySym       keysym   = XLookupKeysym(&keyEvent, 0);

    // 转换为可读字符串
    std::string modStr      = modifierToString(modifier);
    std::string keyStr      = keysymToString(keysym);
    std::string eventType   = (event.type == KeyPress) ? "按下" : "释放";
    std::string fullKeyComb = modStr.empty() ? keyStr : modStr + "+" + keyStr;

    // 单行拼接（核心简化点）：事件类型 | 组合键 | 原始keycode | KeySym
    oss << "按键事件：" << eventType << " | " << fullKeyComb;

    return oss.str();
}

KeySym toLowerKeySym(KeySym sym)
{
    if (sym >= XK_A && sym <= XK_Z)
    {
        return sym + 0x20;
    }
    return sym;
}

bool cstring2ul(const char *str, unsigned long &value)
{
    char *endptr;  // 用于检测转换是否完整
    errno = 0;     // 重置错误码
    value = std::strtoul(str, &endptr, 10);
    if (errno != 0)
    {
        std::cerr << "错误：数值超出unsigned long范围！" << std::endl;
        return false;
    }
    if (*endptr != '\0')
    {
        std::cerr << "错误：输入的不是有效的unsigned long数值！输入内容：" << str << std::endl;
        return false;
    }
    return true;
}

bool stringToKeysym(const std::string &str, KeySym &key)
{
    // 步骤1：处理输入字符串，去掉前缀 "XK_"
    std::string       pure_name;
    const std::string prefix = "XK_";
    if (str.substr(0, prefix.size()) == prefix)
    {
        pure_name = str.substr(prefix.size());  // 截取 XK_ 之后的部分
    }
    else
    {
        pure_name = str;  // 如果没有前缀，直接使用原字符串
    }

    // 步骤2：调用 X11 核心函数转换字符串为 KeySym
    KeySym keysym = XStringToKeysym(pure_name.c_str());

    // 步骤3：错误处理 - 转换失败时 keysym 会返回 NoSymbol
    if (keysym == NoSymbol)
    {
        std::cerr << "Invalid KeySym string: " << str << std::endl;
        return false;
    }
    key = keysym;
    return true;
}

bool simulatePressKeyWithMask(const std::vector<KeySym> &maskKeys, KeySym targetKey, int delayMs)
{
    // 1. 打开X服务器连接
    Display *display = XOpenDisplay(nullptr);
    if (!display)
    {
        fprintf(stderr, "无法连接到X服务器（检查DISPLAY环境变量）\n");
        return false;
    }

    // 转换延迟为微秒
    const int delayUs = delayMs * 1000;

    // 2. 验证所有掩码键并转换为键码
    std::vector<KeyCode> maskKeyCodes;
    for (KeySym maskSym : maskKeys)
    {
        KeyCode code = XKeysymToKeycode(display, maskSym);
        if (code == 0)
        {
            fprintf(stderr, "无效的掩码键（Keysym：%u）\n", (unsigned int)maskSym);
            XCloseDisplay(display);
            return false;
        }
        maskKeyCodes.push_back(code);
    }

    // 3. 验证目标键并转换为键码
    KeyCode targetCode = XKeysymToKeycode(display, targetKey);
    if (targetCode == 0)
    {
        fprintf(stderr, "无效的目标键（Keysym：%u）\n", (unsigned int)targetKey);
        XCloseDisplay(display);
        return false;
    }

    // 打印按下信息
    std::string info;
    for (auto mask : maskKeys)
    {
        info += keysymToString(mask) + " + ";
    }
    info += keysymToString(targetKey) + " [仅按下]";
    fprintf(stdout, "模拟按键：%s\n", info.c_str());

    // 4. 按下所有掩码键（如Shift、Ctrl）
    for (KeyCode code : maskKeyCodes)
    {
        XTestFakeKeyEvent(display, code, True, 0);  // True = 按下
        XFlush(display);
        usleep(delayUs);
    }

    // 5. 按下目标键（不释放）
    XTestFakeKeyEvent(display, targetCode, True, 0);  // True = 按下
    XFlush(display);
    usleep(delayUs);

    // 6. 关闭连接（X11事件已发送，连接可关闭）
    XCloseDisplay(display);
    return true;
}

bool simulateReleaseKeyWithMask(const std::vector<KeySym> &maskKeys, KeySym targetKey, int delayMs)
{
    // 1. 打开X服务器连接
    Display *display = XOpenDisplay(nullptr);
    if (!display)
    {
        fprintf(stderr, "无法连接到X服务器（检查DISPLAY环境变量）\n");
        return false;
    }

    // 转换延迟为微秒
    const int delayUs = delayMs * 1000;

    // 2. 转换掩码键为键码（无需重复验证，按下时已验证）
    std::vector<KeyCode> maskKeyCodes;
    for (KeySym maskSym : maskKeys)
    {
        KeyCode code = XKeysymToKeycode(display, maskSym);
        if (code == 0)
        {
            fprintf(stderr, "释放时发现无效的掩码键（Keysym：%u）\n", (unsigned int)maskSym);
            XCloseDisplay(display);
            return false;
        }
        maskKeyCodes.push_back(code);
    }

    // 3. 转换目标键为键码
    KeyCode targetCode = XKeysymToKeycode(display, targetKey);
    if (targetCode == 0)
    {
        fprintf(stderr, "释放时发现无效的目标键（Keysym：%u）\n", (unsigned int)targetKey);
        XCloseDisplay(display);
        return false;
    }

    // 打印释放信息
    std::string info;
    for (auto mask : maskKeys)
    {
        info += keysymToString(mask) + " + ";
    }
    info += keysymToString(targetKey) + " [仅释放]";
    fprintf(stdout, "模拟按键：%s\n", info.c_str());

    // 4. 释放目标键（先释放目标键，再释放掩码键）
    XTestFakeKeyEvent(display, targetCode, False, 0);  // False = 释放
    XFlush(display);
    usleep(delayUs);

    // 5. 释放所有掩码键（与按下顺序相反）
    for (auto it = maskKeyCodes.rbegin(); it != maskKeyCodes.rend(); ++it)
    {
        XTestFakeKeyEvent(display, *it, False, 0);  // False = 释放
        XFlush(display);
        usleep(delayUs);
    }

    // 6. 关闭连接
    XCloseDisplay(display);
    return true;
}
