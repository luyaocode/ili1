#ifndef CLIENTINFO_H
#define CLIENTINFO_H
#include <QWebSocket>
// 客户端信息结构体

struct ClientInfo
{
    QWebSocket *socket;                   // 客户端连接
    int         mouseX          = 0;      // 客户端鼠标X坐标（相对图片）
    int         mouseY          = 0;      // 客户端鼠标Y坐标（相对图片）
    int         screenWidth     = 1;      // 客户端屏幕宽度
    int         screenHeight    = 1;      // 客户端屏幕高度
    bool        isLeftPressed   = false;  // 左键按压状态
    bool        isRightPressed  = false;  // 右键按压状态
    bool        isMiddlePressed = false;  // 中键按压状态
    // 新增键盘状态跟踪
    bool isCtrlPressed  = false;
    bool isShiftPressed = false;
    bool isAltPressed   = false;
    bool isMetaPressed  = false;
};
#endif // CLIENTINFO_H
