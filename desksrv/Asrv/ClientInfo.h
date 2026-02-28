#ifndef CLIENTINFO_H
#define CLIENTINFO_H
#include <QWebSocket>
#include <QPixmap>
#include <QRect>
#include "commontool/imagetool.h"
// 客户端信息结构体

QString getHdLevelString(HdLevel level);
HdLevel getHdLevelFromString(const QString &level);

struct ClientInfo
{
    QWebSocket *socket;  // 客户端连接
    // ========== 新增：UDP相关信息,用于向客户端推送图片(仅限用于桌面端,浏览器不支持UDP,除非使用webrtc,服务端还没有做)==========
    QHostAddress udpClientIp;           // 客户端UDP接收IP
    quint16      udpClientPort = 8890;  // 客户端UDP接收端口（默认8890）
    QString      userAgent;

    int  mouseX          = 0;      // 客户端鼠标X坐标（相对图片）
    int  mouseY          = 0;      // 客户端鼠标Y坐标（相对图片）
    int  screenWidth     = 1;      // 客户端屏幕宽度
    int  screenHeight    = 1;      // 客户端屏幕高度
    bool isLeftPressed   = false;  // 左键按压状态
    bool isRightPressed  = false;  // 右键按压状态
    bool isMiddlePressed = false;  // 中键按压状态
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
    int     targetFps     = 30;    // 目标帧率（默认30帧）
    int     minFps        = 15;    // 最低帧率（固定15帧）
    int     maxFps        = 60;    // 最高帧率（可配置）
    qint64  lastSendTime  = 0;     // 上一次发送帧的时间戳（ms）
    qreal   diffAreaRatio = 0.0;   // 差分区域占屏比（0~1）

    HdLevel hdLevel       = HdLevel::HdLevel_PngLossless;  // 高清级别
    int     specialEffect = 0;                             // 特效
    int     screenIndex   = 0;
};
#endif  // CLIENTINFO_H
