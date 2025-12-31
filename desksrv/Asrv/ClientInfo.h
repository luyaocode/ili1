#ifndef CLIENTINFO_H
#define CLIENTINFO_H
#include <QWebSocket>
#include <QPixmap>
#include <QRect>
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

    // ========== 新增：每个客户端独立的差分状态 ==========
    QPixmap prevPixmap;            // 该客户端的上一帧截图（独立存储）
    QRect   diffRect;              // 该客户端的差分区域
    bool    isFirstFrame  = true;  // 该客户端是否是第一帧
    int     diffThreshold = 10;    // 该客户端的像素差异阈值（可按需单独调整）
};
#endif  // CLIENTINFO_H
