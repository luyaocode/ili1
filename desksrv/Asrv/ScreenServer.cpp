#include "ScreenServer.h"
#include <QHostAddress>
#include <QDebug>
#include <QImage>
#include <QApplication>
#include <QThread>

#include "commontool/globaltool.h"
#include "x11tool.h"

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

    // 初始化客户端信息
    ClientInfo info;
    info.socket = client;
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

    // 1. 截取屏幕（Qt跨平台方式）
    QScreen *screen       = QApplication::primaryScreen();
    QPixmap  pixmap       = screen->grabWindow(0);  // 截取整个屏幕
    int      screenWidth  = pixmap.width();
    int      screenHeight = pixmap.height();

    // 2. 为每个客户端绘制专属鼠标（按相对坐标转换）
    for (auto client : m_clientMap.keys())
    {
        ClientInfo &info         = m_clientMap[client];
        QPixmap     clientPixmap = pixmap.copy();  // 拷贝截屏，避免多客户端互相影响

        // 3. 绘制虚拟鼠标
        QPainter painter(&clientPixmap);
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

        //        // 4. 转换为Base64编码
        //        QByteArray byteArray;
        //        QBuffer    buffer(&byteArray);
        //        buffer.open(QIODevice::WriteOnly);
        //        clientPixmap.save(&buffer, "PNG", 80);  // 80%质量压缩
        //        QString base64Str = byteArray.toBase64();

        //        // 5. 构造JSON消息推送
        //        QJsonObject jsonObj;
        //        jsonObj["type"]   = "screen_frame";
        //        jsonObj["base64"] = base64Str;
        //        jsonObj["width"]  = screenWidth;
        //        jsonObj["height"] = screenHeight;

        //        QJsonDocument jsonDoc(jsonObj);
        //        sendMessageToClient(client, jsonDoc.toJson(QJsonDocument::Compact));

        // ========== 关键修改：转为JPEG二进制 ==========
        // 4. 将Pixmap转为JPEG格式的二进制数据
        QImage     image = clientPixmap.toImage();
        QByteArray jpegData;
        QBuffer    buffer(&jpegData);
        buffer.open(QIODevice::WriteOnly);
        // 保存为JPEG，质量可调整（0-100）
        image.save(&buffer, "JPEG", 85);  // 85%质量，平衡画质和体积

        // 5. 先发送帧信息（JSON文本），再发送JPEG二进制
        // 帧信息JSON
        QJsonObject frameInfo;
        frameInfo["type"]   = "screen_frame_meta";
        frameInfo["width"]  = screenWidth;
        frameInfo["height"] = screenHeight;
        frameInfo["size"]   = jpegData.size();  // 告知前端二进制数据长度

        QJsonDocument infoDoc(frameInfo);
        sendMessageToClient(client, infoDoc.toJson(QJsonDocument::Compact));

        // 发送JPEG二进制数据
        sendBinaryToClient(client, jpegData);

        //        qDebug() << "Send JPEG frame to client:" << screenWidth << "x" << screenHeight << "Size:" << jpegData.size()
        //                 << "bytes";
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
        qDebug() << "Client" << client->peerAddress().toString() << "mouse move to:" << info.mouseX << ","
                 << info.mouseY << " wh=" << info.screenWidth << "x" << info.screenHeight;
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

        qDebug() << "Client" << client->peerAddress().toString() << "mouse" << button << action;
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
        qDebug() << "Client" << client->peerAddress().toString() << "mouse wheel:" << direction << "steps:" << steps
                 << "at" << x << "," << y;
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
        qDebug() << "Client" << client->peerAddress().toString() << "modifier key" << keyStr << action;
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
        qDebug() << "Client" << client->peerAddress().toString() << "keyboard press:" << keyStr
                 << "(original:" << rawKey << ")"
                 << "modifiers: Ctrl=" << info.isCtrlPressed << " Shift=" << info.isShiftPressed
                 << " Alt=" << info.isAltPressed << " Meta=" << info.isMetaPressed << " success:" << success;
    }
    else if (action == "release")
    {
        bool success = true;
        // 日志输出
        qDebug() << "Client" << client->peerAddress().toString() << "keyboard release:" << keyStr
                 << "(original:" << rawKey << ")"
                 << " success:" << success;
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
