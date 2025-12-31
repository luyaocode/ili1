#include "ScreenServer.h"
#include <QHostAddress>
#include <QDebug>
#include <QImage>
#include <QApplication>
#include <QThread>

#include "commontool/globaltool.h"
#include "x11tool.h"
#include "commontool/screenshooter.h"
#include "commontool/globaldef.h"

ScreenServer::ScreenServer(QObject *parent): QObject(parent)
{
    // 初始化截屏定时器（30帧/秒，可调整）
    m_captureTimer = new QTimer(this);
    //    m_captureTimer->setInterval(33);  // ~30fps
    m_captureTimer->setInterval(15);  // ~60fps
    connect(m_captureTimer, &QTimer::timeout, this, &ScreenServer::captureScreenAndPush);

    // 初始化截屏进程
    m_screenshotProcess = new QProcess(this);
    connect(m_screenshotProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &ScreenServer::onProcessFinished);
}

ScreenServer::~ScreenServer()
{
    stopListen();
    if (m_screenshotProcess)
    {
        m_screenshotProcess->kill();
        m_screenshotProcess->deleteLater();
    }
}

bool ScreenServer::startListen(const QString &serverIp, quint16 serverPort)
{
    if (m_wsServer)
    {
        stopListen();
    }

    m_listenIp   = serverIp;
    m_listenPort = serverPort;

    // 创建WebSocket服务器
    m_wsServer = new QWebSocketServer("ScreenServer", QWebSocketServer::NonSecureMode, this);
    if (!m_wsServer->listen(QHostAddress(serverIp), serverPort))
    {
        qCritical() << "WebSocket server listen failed:" << m_wsServer->errorString();
        return false;
    }

    // 连接信号槽
    connect(m_wsServer, &QWebSocketServer::newConnection, this, &ScreenServer::onNewConnection);
    qInfo() << "WebSocket server start listen on" << serverIp << ":" << serverPort;

    // 启动截屏定时器
    m_captureTimer->start();
    return true;
}

void ScreenServer::stopListen()
{
    if (m_captureTimer)
    {
        m_captureTimer->stop();
    }

    if (m_wsServer)
    {
        // 关闭所有客户端连接
        for (auto client : m_clientMap.keys())
        {
            closeClientConnection(client);
        }
        m_clientMap.clear();

        m_wsServer->close();
        m_wsServer->deleteLater();
        m_wsServer = nullptr;
    }

    qInfo() << "WebSocket server stop listen";
}

void ScreenServer::onNewConnection()
{
    QWebSocket *client = m_wsServer->nextPendingConnection();
    if (!client)
    {
        return;
    }

    qInfo() << "New client connected:" << client->peerAddress().toString();

    // 初始化客户端信息（差分字段已默认初始化：isFirstFrame=true）
    ClientInfo info;
    info.socket = client;
    // 可自定义该客户端的差分阈值（如低带宽客户端调大阈值）
    info.diffThreshold = 10;
    m_clientMap.insert(client, info);

    // 连接客户端信号
    connect(client, &QWebSocket::disconnected, this, &ScreenServer::onClientDisconnected);
    connect(client, &QWebSocket::textMessageReceived, this, &ScreenServer::onTextMessageReceived);
    connect(client, &QWebSocket::binaryMessageReceived, this, &ScreenServer::onBinaryReceived);
}

void ScreenServer::onClientDisconnected()
{
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    if (!client)
    {
        return;
    }

    qInfo() << "Client disconnected:" << client->peerAddress().toString();
    closeClientConnection(client);
}

void ScreenServer::onTextMessageReceived(const QString &message)
{
    QWebSocket *client = qobject_cast<QWebSocket *>(sender());
    if (!client)
    {
        return;
    }

    // 解析JSON格式的鼠标事件
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toUtf8());
    if (!jsonDoc.isObject())
    {
        qWarning() << "Invalid JSON message:" << message;
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (jsonObj["type"].toString().contains("mouse"))
    {
        handleMouseEvent(jsonObj, client);
    }
    else if (jsonObj["type"].toString().contains("keyboard"))
    {
        handleKeyboardEvent(jsonObj, client);
    }
}

void ScreenServer::onBinaryReceived(const QByteArray &binary)
{
    Q_UNUSED(binary);
    qWarning() << "Binary message not supported";
}

void ScreenServer::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    // 截屏进程结束，无特殊处理
}

void ScreenServer::closeClientConnection(QWebSocket *client)
{
    if (!client)
    {
        return;
    }

    client->abort();
    client->deleteLater();
    m_clientMap.remove(client);
}

void ScreenServer::sendMessageToClient(QWebSocket *client, const QString &msg)
{
    if (!client || client->state() != QAbstractSocket::ConnectedState)
    {
        return;
    }
    client->sendTextMessage(msg);
}

void ScreenServer::sendBinaryToClient(QWebSocket *client, const QByteArray &binary)
{
    if (!client || client->state() != QAbstractSocket::ConnectedState)
    {
        return;
    }
    client->sendBinaryMessage(binary);
}

void ScreenServer::captureScreenAndPush()
{
    if (m_clientMap.isEmpty())
    {
        return;  // 无客户端，跳过截屏
    }

    // 1. 截取当前屏幕
    std::future<QPixmap> pixmapFuture       = ScreenShooter::instance()->captureScreenAsync();
    QPixmap              currPixmap         = pixmapFuture.get();
    int                  globalScreenWidth  = currPixmap.width();
    int                  globalScreenHeight = currPixmap.height();
    // 5. 为每个客户端推送差分数据
    for (auto client : m_clientMap.keys())
    {
        ClientInfo &info = m_clientMap[client];  // 单个客户端的专属状态
        QRect       diffRect;

        // 2.1 该客户端的差分区域计算（独立判断首帧）
        if (info.isFirstFrame || info.prevPixmap.isNull() || info.prevPixmap.size() != currPixmap.size())
        {
            // 该客户端首帧/分辨率变化：发送全屏
            diffRect          = QRect(0, 0, globalScreenWidth, globalScreenHeight);
            info.isFirstFrame = false;
            info.prevPixmap   = currPixmap.copy();  // 初始化该客户端的上一帧
        }
        else
        {
            // 对比该客户端的上一帧和当前帧
            diffRect = calculateDiffRect(info.prevPixmap, currPixmap, info.diffThreshold);
            if (diffRect.isEmpty())
            {
                continue;  // 该客户端无变化，跳过推送
            }
            info.prevPixmap = currPixmap.copy();  // 更新该客户端的上一帧
        }

        // 2.2 裁剪该客户端的差分区域
        QPixmap diffPixmap = currPixmap.copy(diffRect);

        // 2.3 绘制该客户端的专属鼠标
        QPixmap clientPixmap = diffPixmap.copy();
        if (diffRect.contains(info.mouseX, info.mouseY))
        {
            // 差分传输 鼠标会有残影
//            drawVirtualMouse(info, globalScreenWidth, globalScreenHeight, clientPixmap);
        }

        // 2.4 构造该客户端的差分帧信息
        QJsonObject frameInfo;
        frameInfo["type"]    = "screen_frame_meta";
        frameInfo["width"]   = globalScreenWidth;
        frameInfo["height"]  = globalScreenHeight;
        frameInfo["diff_x"]  = diffRect.x();
        frameInfo["diff_y"]  = diffRect.y();
        frameInfo["diff_w"]  = diffRect.width();
        frameInfo["diff_h"]  = diffRect.height();
        frameInfo["is_full"] = (diffRect.width() == globalScreenWidth && diffRect.height() == globalScreenHeight);

        // 2.5 发送该客户端的元信息和二进制数据
        QJsonDocument infoDoc(frameInfo);
        sendMessageToClient(client, infoDoc.toJson(QJsonDocument::Compact));

        QImage     diffImage = clientPixmap.toImage();
        QByteArray jpegData;
        QBuffer    buffer(&jpegData);
        buffer.open(QIODevice::WriteOnly);
        diffImage.save(&buffer, "JPEG", 100);
        sendBinaryToClient(client, jpegData);
    }
}

void ScreenServer::handleMouseEvent(const QJsonObject &mouseEvent, QWebSocket *client)
{
    if (!client || !m_clientMap.contains(client))
    {
        return;
    }

    ClientInfo &info = m_clientMap[client];
    QString     type = mouseEvent["type"].toString();

    // 处理鼠标移动
    if (type == "mouse_move")
    {
        info.mouseX       = mouseEvent["x"].toInt();
        info.mouseY       = mouseEvent["y"].toInt();
        info.screenWidth  = mouseEvent["screen_width"].toInt();
        info.screenHeight = mouseEvent["screen_height"].toInt();

        // 模拟移动
        int x, y;
        getRealXY(info, x, y);
        MouseSimulator::getInstance()->moveMouse(x, y);
        QString logStr = QString("Client %1 mouse move to: %2,%3 wh=%4x%5")
                             .arg(client->peerAddress().toString())  // %1: 客户端地址
                             .arg(info.mouseX)                       // %2: 鼠标X坐标
                             .arg(info.mouseY)                       // %3: 鼠标Y坐标
                             .arg(info.screenWidth)                  // %4: 屏幕宽度
                             .arg(info.screenHeight);                // %5: 屏幕高度
        LOG_MESSAGE("Asrv", logStr)
    }
    // 处理鼠标点击
    else if (type == "mouse_click")
    {
        QString button  = mouseEvent["button"].toString();
        QString action  = mouseEvent["action"].toString();
        bool    pressed = action == "press";
        // 模拟点击
        int x, y;
        getRealXY(info, x, y);
        MouseSimulator::MouseButton btn = MouseSimulator::MouseButton::ButtonNone;
        if (pressed)
        {
            if (button == "left")
            {
                info.isLeftPressed = pressed;
                btn                = MouseSimulator::MouseButton::LeftButton;
            }
            else if (button == "right")
            {
                info.isRightPressed = pressed;
                btn                 = MouseSimulator::MouseButton::RightButton;
            }
            else if (button == "middle")
            {
                info.isMiddlePressed = pressed;
                btn                  = MouseSimulator::MouseButton::MiddleButton;
            }
            MouseSimulator::getInstance()->pressMouse(btn, x, y);
        }
        else
        {
            if (button == "left")
            {
                info.isLeftPressed = pressed;
                btn                = MouseSimulator::MouseButton::LeftButton;
            }
            else if (button == "right")
            {
                info.isRightPressed = pressed;
                btn                 = MouseSimulator::MouseButton::RightButton;
            }
            else if (button == "middle")
            {
                info.isMiddlePressed = pressed;
                btn                  = MouseSimulator::MouseButton::MiddleButton;
            }
            MouseSimulator::getInstance()->releaseMouse(btn, x, y);
        }
        QString logStr =
            QString("Client %1 mouse  %2 %3").arg(client->peerAddress().toString()).arg(button).arg(action);

        LOG_MESSAGE("Asrv", logStr)
    }
    // 新增：处理滚轮事件
    else if (type == "mouse_wheel")
    {
        QString direction = mouseEvent["direction"].toString();
        int     steps     = mouseEvent["steps"].toInt();
        int     x         = mouseEvent["x"].toInt();
        int     y         = mouseEvent["y"].toInt();
        // 模拟Linux系统滚轮
        MouseSimulator::WheelDirection eDirection = getScrollWhellDirection(direction);
        MouseSimulator::getInstance()->scrollWheel(eDirection, steps);
        QString logStr = QString("Client %1 mouse wheel: %2 steps: %3 at %4,%5")
                             .arg(client->peerAddress().toString())  // %1: 客户端地址
                             .arg(direction)                         // %2: 滚轮方向
                             .arg(steps)                             // %3: 滚动步数
                             .arg(x)                                 // %4: X坐标
                             .arg(y);                                // %5: Y坐标
        LOG_MESSAGE("Asrv", logStr)
    }
}

// 补全键盘事件处理逻辑
void ScreenServer::handleKeyboardEvent(const QJsonObject &keyboardEvent, QWebSocket *client)
{
    // 校验客户端有效性
    if (!client || !m_clientMap.contains(client))
    {
        qWarning() << "Invalid client for keyboard event";
        return;
    }

    ClientInfo &info = m_clientMap[client];
    QString     type = keyboardEvent["type"].toString();

    // 只处理键盘类型事件
    if (type != "keyboard")
    {
        qWarning() << "Not a keyboard event type:" << type;
        return;
    }

    // 提取核心参数
    QString     action    = keyboardEvent["action"].toString();     // press/release
    QString     keyStr    = keyboardEvent["key"].toString();        // 标准化的按键名
    QJsonObject modifiers = keyboardEvent["modifiers"].toObject();  // 组合键状态
    QString     rawKey    = keyboardEvent["rawKey"].toString();     // 新增：前端传的原始符号键（如!）

    // 更新客户端的组合键状态
    info.isCtrlPressed  = modifiers["ctrl"].toBool();
    info.isShiftPressed = modifiers["shift"].toBool();
    info.isAltPressed   = modifiers["alt"].toBool();
    info.isMetaPressed  = modifiers["meta"].toBool();

    // 转换为std::string方便处理
    std::string keyName = keyStr.toStdString();

    // 跳过纯组合键的press/release（单独处理组合键状态即可）
    if (keyName == "ctrl" || keyName == "shift" || keyName == "alt" || keyName == "meta")
    {
        QString logStr =
            QString("Client %1 modifier key %2 %3").arg(client->peerAddress().toString()).arg(keyStr).arg(action);
        LOG_MESSAGE("Asrv", logStr)
        return;
    }

    // 转换按键名到X11 KeySym
    KeySym targetKey = getKeySymFromName(keyName, info.isShiftPressed);
    if (targetKey == NoSymbol)
    {
        qWarning() << "Unsupported key:" << keyStr;
        return;
    }

    // 只处理press事件（release事件由simulateKeyWithMask内部处理）
    if (action == "press")
    {
        // 构建组合键掩码列表
        std::vector<KeySym> maskKeys = buildMaskKeys(info);

        // 模拟按键（包含组合键）
        bool success = simulateKeyWithMask(maskKeys, targetKey);

        // 日志输出（补充符号键信息）
        QString logStr =
            QString(
                "Client %1 keyboard press: %2 (original: %3) modifiers: Ctrl=%4 Shift=%5 Alt=%6 Meta=%7 success: %8")
                .arg(client->peerAddress().toString())  // %1: 客户端地址
                .arg(keyStr)                            // %2: 按键字符串
                .arg(rawKey)                            // %3: 原始按键值
                .arg(info.isCtrlPressed)                // %4: Ctrl是否按下
                .arg(info.isShiftPressed)               // %5: Shift是否按下
                .arg(info.isAltPressed)                 // %6: Alt是否按下
                .arg(info.isMetaPressed)                // %7: Meta是否按下
                .arg(success);                          // %8: 操作是否成功
        LOG_MESSAGE("Asrv", logStr)
    }
    else if (action == "release")
    {
        bool success = true;
        // 日志输出
        QString logStr = QString("Client %1 keyboard release: %2 (original: %3)  success: %4")
                             .arg(client->peerAddress().toString())  // %1: 客户端地址
                             .arg(keyStr)                            // %2: 按键字符串
                             .arg(rawKey)                            // %3: 原始按键值
                             .arg(success);                          // %4: 操作是否成功
        LOG_MESSAGE("Asrv", logStr)
    }
}

void ScreenServer::getRealXY(const ClientInfo &info, int &x, int &y)
{
    QScreen *screen       = QApplication::primaryScreen();
    int      screenWidth  = screen->size().width();
    int      screenHeight = screen->size().height();
    // 转换相对坐标到屏幕绝对坐标
    x = info.mouseX * screenWidth / info.screenWidth;
    y = info.mouseY * screenHeight / info.screenHeight;
}

MouseSimulator::WheelDirection ScreenServer::getScrollWhellDirection(const QString &direction)
{
    if (direction == "up")
    {
        return MouseSimulator::WheelDirection::WheelUp;
    }
    else if (direction == "down")
    {
        return MouseSimulator::WheelDirection::WheelDown;
    }
    else if (direction == "left")
    {
        return MouseSimulator::WheelDirection::WheelLeft;
    }
    else if (direction == "right")
    {
        return MouseSimulator::WheelDirection::WheelRight;
    }
    return MouseSimulator::WheelDirection::WheelNone;
}

void ScreenServer::drawVirtualMouse(const ClientInfo &info,
                                    const int         screenWidth,
                                    const int         screenHeight,
                                    QPixmap          &pixmap)
{
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // 转换相对坐标到屏幕绝对坐标
    int mouseX = info.mouseX * screenWidth / info.screenWidth;  // 前端传的相对宽度=屏幕宽度时直接用
    int mouseY = info.mouseY * screenHeight / info.screenHeight;

    // 绘制鼠标（箭头样式）
    int          mouseSize = 20;
    QPainterPath mousePath;
    mousePath.moveTo(mouseX, mouseY);
    mousePath.lineTo(mouseX + mouseSize, mouseY + mouseSize / 2);
    mousePath.lineTo(mouseX + mouseSize / 3, mouseY + mouseSize);
    mousePath.lineTo(mouseX, mouseY + mouseSize / 3);
    mousePath.closeSubpath();

    // 按压状态样式
    if (info.isLeftPressed)
    {
        painter.setBrush(QColor(255, 0, 0, 180));
        painter.setPen(QPen(Qt::red, 2));
    }
    else if (info.isRightPressed)
    {
        painter.setBrush(QColor(0, 0, 255, 180));
        painter.setPen(QPen(Qt::blue, 2));
    }
    else
    {
        painter.setBrush(QColor(0, 0, 0, 180));
        painter.setPen(QPen(Qt::black, 2));
    }

    painter.drawPath(mousePath);
    // 白色描边增强辨识度
    painter.setBrush(Qt::transparent);
    painter.setPen(QPen(Qt::white, 1));
    painter.drawPath(mousePath);
}

QRect ScreenServer::calculateDiffRect(const QPixmap &prev, const QPixmap &curr, int threshold)
{
    QImage prevImg = prev.toImage().convertToFormat(QImage::Format_RGB888);
    QImage currImg = curr.toImage().convertToFormat(QImage::Format_RGB888);

    int  width  = prevImg.width();
    int  height = prevImg.height();
    int  minX = width, minY = height, maxX = 0, maxY = 0;
    bool hasDiff = false;

    // 逐像素对比（可优化：按块对比提升性能）
    for (int y = 0; y < height; y++)
    {
        uchar *prevLine = prevImg.scanLine(y);
        uchar *currLine = currImg.scanLine(y);
        for (int x = 0; x < width; x++)
        {
            // 计算RGB差值
            int rDiff     = abs(prevLine[x * 3] - currLine[x * 3]);
            int gDiff     = abs(prevLine[x * 3 + 1] - currLine[x * 3 + 1]);
            int bDiff     = abs(prevLine[x * 3 + 2] - currLine[x * 3 + 2]);
            int totalDiff = rDiff + gDiff + bDiff;

            if (totalDiff > threshold)
            {
                // 更新差分区域边界
                minX    = qMin(minX, x);
                minY    = qMin(minY, y);
                maxX    = qMax(maxX, x);
                maxY    = qMax(maxY, y);
                hasDiff = true;
            }
        }
    }

    if (!hasDiff)
    {
        return QRect();  // 无变化
    }

    // 扩展差分区域（避免边缘截断，可选）
    minX = qMax(0, minX - 2);
    minY = qMax(0, minY - 2);
    maxX = qMin(width - 1, maxX + 2);
    maxY = qMin(height - 1, maxY + 2);

    return QRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}
