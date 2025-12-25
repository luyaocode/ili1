#include "ClientHandler.h"
#include "TcpServer.h"
#include <QDebug>
#include <QString>
#include <QDateTime>

// ================= ClientHandler 实现 =================
ClientHandler::ClientHandler(qintptr socketDescriptor, QObject *parent)
    : QObject(parent)
    , m_socketDescriptor(socketDescriptor)
    , m_tcpSocket(nullptr)
    , m_webSocket(nullptr)
{
    // 初始化TCP Socket
    m_tcpSocket = new QTcpSocket(this);
    m_tcpSocket->setSocketDescriptor(m_socketDescriptor);

    qDebug() << "新客户端连接：IP=" << m_tcpSocket->peerAddress().toString()
             << "，Socket描述符=" << socketDescriptor;

    // ========== 配置Socket适配大文件 ==========
    m_tcpSocket->setReadBufferSize(0); // 禁用读缓冲区大小限制
    m_tcpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, MAX_REQUEST_SIZE / 10);
    m_tcpSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    // 关联信号
    connect(m_tcpSocket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_tcpSocket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);
    connect(m_tcpSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &ClientHandler::onSocketError);
}

ClientHandler::~ClientHandler()
{
    if (m_webSocket) {
        m_webSocket->close();
        m_webSocket->deleteLater();
    }
    if (m_tcpSocket) {
        m_tcpSocket->deleteLater();
    }
    qDebug() << "客户端连接释放：Socket描述符=" << m_socketDescriptor;
}

void ClientHandler::onReadyRead()
{
    // 读取所有数据到缓冲区
    m_buffer += m_tcpSocket->readAll();

    // 若已升级为WebSocket，不再处理TCP数据
    if (m_webSocket) {
        return;
    }

    // ========== 关键：判断是否是WebSocket握手请求 ==========
    if (m_buffer.contains("Upgrade: websocket", Qt::CaseInsensitive)) {
        // 处理WebSocket握手
        if (handleWebSocketHandshake(m_tcpSocket, m_buffer)) {
            // 握手成功后，清空缓冲区，后续由WebSocket处理
            m_buffer.clear();
        } else {
            // 握手失败，关闭连接
            m_tcpSocket->write("HTTP/1.1 400 Bad Request\r\n\r\n");
            m_tcpSocket->flush();
            m_tcpSocket->close();
        }
    } else {
        // 处理普通HTTP请求（原逻辑）
        handleHttpRequest(m_tcpSocket, m_buffer);
    }
}

void ClientHandler::onDisconnected()
{
    this->deleteLater(); // 释放当前处理类
}

void ClientHandler::onDisconnected()
{
    this->deleteLater(); // 释放当前处理类
}

void ClientHandler::onSocketError(QAbstractSocket::SocketError error)
{
    qWarning() << "客户端连接错误：" << error << m_tcpSocket->errorString();
    this->deleteLater();
}

void ClientHandler::onWebSocketTextMessageReceived(const QString &message)
{
    qDebug() << "收到WebSocket文本消息：" << message;
    // 处理WebSocket消息（例如转发到终端进程）
    m_webSocket->sendTextMessage("服务器响应：" + message);
}

void ClientHandler::onWebSocketBinaryMessageReceived(const QByteArray &data)
{
    qDebug() << "收到WebSocket二进制消息，长度：" << data.size();
    // 处理二进制消息
    m_webSocket->sendBinaryMessage(data);
}

void ClientHandler::onWebSocketDisconnected()
{
    qDebug() << "WebSocket客户端断开连接：" << m_webSocket->peerAddress().toString();
    m_webSocket->deleteLater();
    m_webSocket = nullptr;
    this->deleteLater();
}

// 处理普通HTTP请求（原业务逻辑，此处仅为示例）
void ClientHandler::handleHttpRequest(QTcpSocket *socket, const QByteArray &data)
{
    qDebug() << "处理HTTP请求，数据长度：" << data.size();
    // 示例：返回简单的HTTP响应
    QString response = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: text/html; charset=utf-8\r\n"
                       "Content-Length: 20\r\n\r\n"
                       "<h1>Hello HTTP Client</h1>";
    socket->write(response.toUtf8());
    socket->flush();
    // 短连接：发送响应后关闭连接
    socket->close();
}
// 处理WebSocket握手（基于RFC6455标准）
bool ClientHandler::handleWebSocketHandshake(QTcpSocket *socket, const QByteArray &data)
{
    // 1. 解析Sec-WebSocket-Key头
    QRegExp reg("Sec-WebSocket-Key: ([\\w+/=]+)\r\n", Qt::CaseInsensitive);
    if (reg.indexIn(data) == -1) {
        qWarning() << "WebSocket握手失败：未找到Sec-WebSocket-Key";
        return false;
    }

    // 2. 计算Sec-WebSocket-Accept
    QString key = reg.cap(1).trimmed();
    const QString magicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    QByteArray sha1 = QCryptographicHash::hash((key + magicString).toUtf8(), QCryptographicHash::Sha1);
    QString acceptKey = sha1.toBase64();

    // 3. 构造握手响应
    QByteArray response = "HTTP/1.1 101 Switching Protocols\r\n"
                          "Upgrade: websocket\r\n"
                          "Connection: Upgrade\r\n"
                          "Sec-WebSocket-Accept: " + acceptKey.toUtf8() + "\r\n\r\n";
    socket->write(response);
    socket->flush();

    // 4. 使用QWebSocket接管Socket描述符（关键！）
    m_webSocket = new QWebSocket();
    if (!m_webSocket->setSocketDescriptor(socket->socketDescriptor())) {
        qWarning() << "WebSocket接管Socket失败：" << m_webSocket->errorString();
        m_webSocket->deleteLater();
        m_webSocket = nullptr;
        return false;
    }

    // 5. 断开原TCP Socket的信号关联，由WebSocket接管
    socket->disconnect();
    socket->setParent(nullptr);
    socket->deleteLater(); // TCP Socket不再使用，释放

    // 6. 关联WebSocket信号
    connect(m_webSocket, &QWebSocket::textMessageReceived,
            this, &ClientHandler::onWebSocketTextMessageReceived);
    connect(m_webSocket, &QWebSocket::binaryMessageReceived,
            this, &ClientHandler::onWebSocketBinaryMessageReceived);
    connect(m_webSocket, &QWebSocket::disconnected,
            this, &ClientHandler::onWebSocketDisconnected);

    qDebug() << "WebSocket握手成功：" << m_webSocket->peerAddress().toString();
    return true;
}
