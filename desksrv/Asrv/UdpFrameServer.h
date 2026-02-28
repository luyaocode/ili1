#ifndef UDPFRAMESERVER_H
#define UDPFRAMESERVER_H

#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <QThread>

// UDP包头部结构体（不变）
struct UdpFrameHeader {
    uint32_t frame_id;
    uint32_t packet_index;
    uint32_t total_packets;
    uint32_t data_len;
    uint32_t diff_x;
    uint32_t diff_y;
    uint32_t diff_w;
    uint32_t diff_h;
    bool     is_full;
};

class UdpFrameServer : public QObject
{
    Q_OBJECT
public:
    // 线程安全的单例获取（懒汉+双重检查锁）
    static UdpFrameServer* getInstance(QObject *parent = nullptr);

    // 初始化UDP服务（指定端口+线程数）
    bool init(quint16 serverPort = 8889, int threadPoolSize = 4);

    // 线程安全的帧发送接口
    void sendFrame(const QByteArray &frameData,
                   const QRect &diffRect,
                   bool isFull,
                   const QHostAddress &clientIp,
                   quint16 clientPort);

    // 停止UDP服务
    void stop();

    // 设置UDP单包最大尺寸
    void setMaxPacketSize(int size) { m_maxPacketSize = size; }
    int maxPacketSize() const { return m_maxPacketSize; }

private:
    explicit UdpFrameServer(QObject *parent = nullptr);
    ~UdpFrameServer() override;

    // 禁止拷贝
    UdpFrameServer(const UdpFrameServer&) = delete;
    UdpFrameServer& operator=(const UdpFrameServer&) = delete;

    // 为当前线程创建/获取对应的UDP Socket
    QUdpSocket* getThreadSocket();

    // 发送单个UDP包（线程安全）
    void sendPacket(QUdpSocket *socket,
                    const UdpFrameHeader &header,
                    const QByteArray &data,
                    const QHostAddress &clientIp,
                    quint16 clientPort);

private:
    // ========== 线程安全相关成员 ==========
    static std::mutex m_instanceMutex;          // 单例初始化锁
    static UdpFrameServer* m_instance;          // 单例实例
    std::mutex m_socketPoolMutex;               // Socket池锁
    std::unordered_map<std::thread::id, QUdpSocket*> m_threadSocketPool; // 线程-Socket映射
    std::atomic<uint32_t> m_frameId {0};         // 原子帧ID（全局唯一）
    int m_maxPacketSize = 1400;                 // UDP单包最大尺寸
    quint16 m_serverPort = 8889;                // 服务端UDP端口
    bool m_isInited = false;                    // 初始化标记
};

#endif // UDPFRAMESERVER_H
