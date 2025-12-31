#ifndef SCREENSHOOTER_H
#define SCREENSHOOTER_H

#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <future>  // 引入std::future/std::async
#include <memory>  // 智能指针

// Linux DRM/X11截屏工具类（单例 + 异步截屏 + 多线程安全）
// 优先使用DRM（GPU帧缓冲区）截屏，失败则回退到X11共享内存方案
class ScreenShooter : public QObject
{
    Q_OBJECT
    // 私有构造/析构：禁止外部实例化
    explicit ScreenShooter(QObject *parent = nullptr);
    ~ScreenShooter() override;

    // 禁用拷贝/赋值：保证单例唯一性
    ScreenShooter(const ScreenShooter &)            = delete;
    ScreenShooter &operator=(const ScreenShooter &) = delete;

public:
    // 线程安全的单例获取接口（双重检查锁）
    static ScreenShooter *instance();

    // 异步截屏接口：返回std::future<QPixmap>，非阻塞
    std::future<QPixmap> captureScreenAsync();

    // 同步截屏接口（兼容原有逻辑，加锁保护）
    QPixmap captureScreen();

    // 获取屏幕分辨率（截屏前调用有效）
    int screenWidth() const
    {
        return m_screenWidth;
    }
    int screenHeight() const
    {
        return m_screenHeight;
    }

    // 检查DRM是否初始化成功
    bool isDrmAvailable() const
    {
        return m_drmInited;
    }

private:
    // DRM相关内部结构体
    struct DrmInfo
    {
        int      fd     = -1;       // DRM设备文件描述符
        void    *fbMap  = nullptr;  // 映射的帧缓冲区内存
        int      width  = 0;        // 屏幕宽度
        int      height = 0;        // 屏幕高度
        int      stride = 0;        // 每行字节数
        int      bpp    = 32;       // 色深（默认32位）
        uint32_t fbId   = 0;        // 帧缓冲区ID
        uint32_t crtcId = 0;        // 新增：CRTC ID（解决字段缺失）
    };

    // 内部同步截屏实现（供异步接口调用）
    QPixmap captureScreenInternal();

    // DRM初始化/清理
    bool initDrmDevice();
    // 新增：遍历系统所有FB，找到匹配当前分辨率的有效FB
    uint32_t findValidFbIdByResolution(int fd, int targetWidth, int targetHeight);

    void cleanupDrmDevice();

    // 底层截屏实现
    QPixmap captureScreenDrm();
    QPixmap captureScreenX11();

private:
    // ======== 单例相关静态成员 ========
    static ScreenShooter *m_instance;       // 单例实例
    static QMutex         m_instanceMutex;  // 单例创建锁

    // ======== 线程安全相关成员 ========
    mutable QMutex m_captureMutex;       // 截屏操作锁（mutable允许const函数使用）
    QPixmap        m_lastFrame;          // 截图缓存（优化高频调用）
    QDateTime      m_lastCaptureTime;    // 最后截屏时间
    const int      m_cacheTimeout = 50;  // 缓存超时（毫秒）

    // ======== 常规成员 ========
    DrmInfo m_drmInfo;               // DRM设备信息
    bool    m_drmInited    = false;  // DRM初始化状态
    int     m_screenWidth  = 0;      // 屏幕宽度
    int     m_screenHeight = 0;      // 屏幕高度
};

#endif  // SCREENSHOOTER_H
