#include "UdpFrameServer.h"
#include <QDataStream>
#include <QDebug>
#include <QRect>
#include <QThread>

// 静态成员初始化
std::mutex      UdpFrameServer::m_instanceMutex;
UdpFrameServer *UdpFrameServer::m_instance = nullptr;

UdpFrameServer::UdpFrameServer(QObject *parent): QObject(parent)
{
}

UdpFrameServer::~UdpFrameServer()
{
    stop();
}

// ========== 核心改造：线程安全的单例获取 ==========
UdpFrameServer *UdpFrameServer::getInstance(QObject *parent)
{
    // 双重检查锁（DCL）：线程安全的懒汉单例
    if (!m_instance)
    {
        std::lock_guard<std::mutex> lock(m_instanceMutex);
        if (!m_instance)
        {
            m_instance = new UdpFrameServer(parent);
        }
    }
    return m_instance;
}

bool UdpFrameServer::init(quint16 serverPort, int threadPoolSize)
{
    std::lock_guard<std::mutex> lock(m_socketPoolMutex);  // 加锁保护初始化

    if (m_isInited)
    {
        qWarning() << "UDPFrameServer already inited";
        return true;
    }

    m_serverPort = serverPort;
    m_isInited   = true;

    qInfo() << "UDPFrameServer init success on port:" << m_serverPort << "thread pool size:" << threadPoolSize;
    return true;
}

// ========== 核心改造：为每个线程分配独立Socket ==========
QUdpSocket *UdpFrameServer::getThreadSocket()
{
    std::lock_guard<std::mutex> lock(m_socketPoolMutex);  // 保护Socket池

    // 获取当前线程ID
    std::thread::id threadId = std::this_thread::get_id();
    // Qt多线程（QThread）兼容：若用QThread，可改用QThread::currentThreadId()
    // Qt::HANDLE threadId = QThread::currentThreadId();

    // 线程已有Socket则直接返回
    if (m_threadSocketPool.count(threadId))
    {
        return m_threadSocketPool[threadId];
    }

    // 线程无Socket则创建新的（绑定端口但不占用，UDP发送端可复用端口）
    QUdpSocket          *socket = new QUdpSocket(this);
    QUdpSocket::BindMode bindMode =
        static_cast<QUdpSocket::BindMode>(QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);

    // 绑定端口（核心：传入 bindMode 标志）
    bool bindOk = socket->bind(QHostAddress::Any, m_serverPort, bindMode);
    if (!bindOk)
    {
        qWarning() << "Thread socket bind failed:" << socket->errorString() << "port:" << m_serverPort;
        // 绑定失败也保留 Socket，使用系统自动分配的临时端口（UDP 发送端可不用绑定）
    }
    else
    {
        qDebug() << "Thread socket bind success on port:" << m_serverPort;
    }

    // 开启低延迟模式（减少发送缓冲区延迟）
    socket->setSocketOption(QAbstractSocket::LowDelayOption, true);

    // 绑定端口（可选，UDP发送端可不绑定，由系统自动分配）
    if (!socket->bind(QHostAddress::Any, m_serverPort, QUdpSocket::ShareAddress))
    {
        qWarning() << "Thread socket bind failed:" << socket->errorString();
        // 绑定失败则不绑定端口，直接使用系统分配的临时端口
    }

    // 将Socket加入线程池
    m_threadSocketPool[threadId] = socket;
    qInfo() << "Create new UDP socket for thread:" << QString::number((qlonglong)&threadId);

    return socket;
}

// ========== 线程安全的帧发送接口 ==========
void UdpFrameServer::sendFrame(
    const QByteArray &frameData, const QRect &diffRect, bool isFull, const QHostAddress &clientIp, quint16 clientPort)
{
    if (!m_isInited || clientIp.isNull() || clientPort == 0 || frameData.isEmpty())
    {
        qWarning() << "UDP send frame failed: invalid param";
        return;
    }

    // 1. 获取当前线程对应的Socket（线程隔离，避免并发冲突）
    QUdpSocket *socket = getThreadSocket();
    if (!socket)
    {
        qWarning() << "Get thread socket failed";
        return;
    }

    // 2. 计算拆包数量
    int      totalDataLen = frameData.size();
    int      totalPackets = (totalDataLen + m_maxPacketSize - 1) / m_maxPacketSize;
    uint32_t frameId      = m_frameId++;  // 原子操作，全局唯一

    // 3. 拆包并发送（每个包都使用当前线程的Socket）
    for (int i = 0; i < totalPackets; i++)
    {
        UdpFrameHeader header;
        header.frame_id      = frameId;
        header.packet_index  = i;
        header.total_packets = totalPackets;
        header.diff_x        = diffRect.x();
        header.diff_y        = diffRect.y();
        header.diff_w        = diffRect.width();
        header.diff_h        = diffRect.height();
        header.is_full       = isFull;

        int offset            = i * m_maxPacketSize;
        header.data_len       = qMin(m_maxPacketSize, totalDataLen - offset);
        QByteArray packetData = frameData.mid(offset, header.data_len);

        // 发送单个包（使用线程专属Socket）
        sendPacket(socket, header, packetData, clientIp, clientPort);
    }
}

void UdpFrameServer::sendPacket(QUdpSocket           *socket,
                                const UdpFrameHeader &header,
                                const QByteArray     &data,
                                const QHostAddress   &clientIp,
                                quint16               clientPort)
{
    // 组装UDP包（不变）
    QByteArray  packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);

    stream << header.frame_id << header.packet_index << header.total_packets << header.data_len << header.diff_x
           << header.diff_y << header.diff_w << header.diff_h << (header.is_full ? (uint8_t)1 : (uint8_t)0);

    packet.append(data);

    // 发送（使用线程专属Socket，无并发冲突）
    qint64 sendLen = socket->writeDatagram(packet, clientIp, clientPort);
    if (sendLen == -1)
    {
        std::thread::id threadId = std::this_thread::get_id(); // 先赋值给变量，避免临时对象
        qWarning() << "UDP send packet failed:" << socket->errorString()
                   << "client:" << clientIp.toString() << ":" << clientPort
                   << "thread:" << QString::number((qlonglong)&threadId);
    }
}

void UdpFrameServer::stop()
{
    std::lock_guard<std::mutex> lock(m_socketPoolMutex);

    if (!m_isInited)
    {
        return;
    }

    // 销毁所有线程的Socket
    for (auto &pair : m_threadSocketPool)
    {
        if (pair.second)
        {
            pair.second->close();
            pair.second->deleteLater();
        }
    }
    m_threadSocketPool.clear();

    m_isInited = false;
    qInfo() << "UDPFrameServer stopped, all thread sockets closed";
}
