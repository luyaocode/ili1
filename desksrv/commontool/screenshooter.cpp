#include "screenshooter.h"
#include <QDebug>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <thread>  // std::thread

// DRM相关头文件
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

// X11相关头文件
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

uint32_t getCrtcIdFromEncoder(int fd, drmModeRes *res, uint32_t encoderId)
{
    Q_UNUSED(res)
    if (encoderId == 0 || !res)
        return 0;

    drmModeEncoder *encoder = drmModeGetEncoder(fd, encoderId);
    if (!encoder)
        return 0;

    uint32_t crtcId = encoder->crtc_id;
    drmModeFreeEncoder(encoder);

    // 验证CRTC ID是否有效
    for (int i = 0; i < res->count_crtcs; i++)
    {
        if (res->crtcs[i] == crtcId)
        {
            return crtcId;
        }
    }
    return 0;
}

// ========== 单例静态成员初始化 ==========
ScreenShooter *ScreenShooter::m_instance = nullptr;
QMutex         ScreenShooter::m_instanceMutex;

// ========== 单例获取接口（线程安全） ==========
ScreenShooter *ScreenShooter::instance()
{
    if (m_instance == nullptr)
    {
        QMutexLocker locker(&m_instanceMutex);
        if (m_instance == nullptr)
        {
            m_instance = new ScreenShooter(nullptr);
        }
    }
    return m_instance;
}

// ========== 私有构造函数 ==========
ScreenShooter::ScreenShooter(QObject *parent): QObject(parent)
{
    // 初始化DRM（优先方案）
    m_drmInited = initDrmDevice();
    if (m_drmInited)
    {
        m_screenWidth  = m_drmInfo.width;
        m_screenHeight = m_drmInfo.height;
        qInfo() << "DRM initialized successfully, screen size:" << m_screenWidth << "x" << m_screenHeight;
    }
    else
    {
        qWarning() << "DRM init failed, will use X11 fallback";
        // 预获取X11屏幕分辨率
        Display *display = XOpenDisplay(nullptr);
        if (display)
        {
            int screen     = DefaultScreen(display);
            m_screenWidth  = DisplayWidth(display, screen);
            m_screenHeight = DisplayHeight(display, screen);
            XCloseDisplay(display);
        }
    }
}

// ========== 析构函数 ==========
ScreenShooter::~ScreenShooter()
{
    // 清理DRM资源
    cleanupDrmDevice();
    qInfo() << "ScreenShooter instance destroyed";
}

// ========== DRM初始化 ==========
bool ScreenShooter::initDrmDevice()
{
    // 重置之前的资源
    if (m_drmInfo.fbMap)
    {
        munmap(m_drmInfo.fbMap, (uint64_t)m_drmInfo.stride * m_drmInfo.height);
        m_drmInfo.fbMap = nullptr;
    }
    if (m_drmInfo.fd >= 0)
    {
        close(m_drmInfo.fd);
        m_drmInfo.fd = -1;
    }
    m_drmInited = false;

    // 1. 打开DRM设备（优先card0，失败则尝试其他card）
    const char *drmDevices[] = {"/dev/dri/card0", "/dev/dri/card1", "/dev/dri/card2"};
    int         drmFd        = -1;
    for (const char *dev : drmDevices)
    {
        drmFd = open(dev, O_RDWR | O_CLOEXEC);
        if (drmFd >= 0)
        {
            m_drmInfo.fd = drmFd;
            qInfo() << "Successfully opened DRM device: " << dev;
            break;
        }
    }
    if (m_drmInfo.fd < 0)
    {
        qWarning() << "Failed to open any DRM device: " << strerror(errno);
        return false;
    }

    // 2. 获取DRM资源
    drmModeRes *res = drmModeGetResources(m_drmInfo.fd);
    if (!res)
    {
        qWarning() << "Failed to get DRM resources: " << strerror(errno);
        close(m_drmInfo.fd);
        m_drmInfo.fd = -1;
        return false;
    }

    // 3. 查找活跃的连接器+对应的CRTC
    drmModeConnector *conn = nullptr;
    drmModeCrtc      *crtc = nullptr;
    drmModeModeInfo   mode {};
    uint32_t          targetCrtcId   = 0;
    bool              foundValidConn = false;

    for (int i = 0; i < res->count_connectors; i++)
    {
        conn = drmModeGetConnector(m_drmInfo.fd, res->connectors[i]);
        if (!conn)
            continue;

        // 只处理已连接的显示器
        if (conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0)
        {
            mode             = conn->modes[0];
            m_drmInfo.width  = mode.hdisplay;
            m_drmInfo.height = mode.vdisplay;

            // 从encoder获取CRTC ID
            targetCrtcId = getCrtcIdFromEncoder(m_drmInfo.fd, res, conn->encoder_id);
            if (targetCrtcId != 0)
            {
                m_drmInfo.crtcId = targetCrtcId;
                foundValidConn   = true;
                break;  // 保留conn不释放，后续使用
            }
        }
        // 未找到有效连接器时释放
        if (!foundValidConn)
        {
            drmModeFreeConnector(conn);
            conn = nullptr;
        }
    }

    if (!foundValidConn || m_drmInfo.crtcId == 0)
    {
        qWarning() << "No connected display or active CRTC found via DRM";
        drmModeFreeResources(res);
        close(m_drmInfo.fd);
        m_drmInfo.fd = -1;
        return false;
    }

    // 4. 尝试从CRTC获取fb_id（旧逻辑）
    crtc          = drmModeGetCrtc(m_drmInfo.fd, m_drmInfo.crtcId);
    uint32_t fbId = 0;
    if (crtc)
    {
        fbId = crtc->buffer_id;
        qInfo() << "FB ID from CRTC: " << fbId;
        // 先尝试用CRTC的fb_id查询
        drmModeFB *fb = drmModeGetFB(m_drmInfo.fd, fbId);
        if (fb)
        {
            // CRTC的fb_id有效，直接使用
            drmModeFreeFB(fb);
            m_drmInfo.fbId = fbId;
        }
        else
        {
            qWarning() << "CRTC FB ID " << fbId << " is invalid: " << strerror(errno);
            // CRTC的fb_id无效，调用兼容版函数查找有效FB
            fbId = findValidFbIdByResolution(m_drmInfo.fd, m_drmInfo.width, m_drmInfo.height);
            if (fbId == 0)
            {
                qWarning() << "No valid FB found in system";
                drmModeFreeCrtc(crtc);
                drmModeFreeConnector(conn);
                drmModeFreeResources(res);
                close(m_drmInfo.fd);
                m_drmInfo.fd = -1;
                return false;
            }
            m_drmInfo.fbId = fbId;
        }
        drmModeFreeCrtc(crtc);
    }
    else
    {
        qWarning() << "Failed to get CRTC, try to find FB by resolution directly";
        // 连CRTC都获取失败，直接按分辨率找FB
        fbId = findValidFbIdByResolution(m_drmInfo.fd, m_drmInfo.width, m_drmInfo.height);
        if (fbId == 0)
        {
            drmModeFreeConnector(conn);
            drmModeFreeResources(res);
            close(m_drmInfo.fd);
            m_drmInfo.fd = -1;
            return false;
        }
        m_drmInfo.fbId = fbId;
    }

    // 5. 获取有效FB的信息
    drmModeFB *fb = drmModeGetFB(m_drmInfo.fd, m_drmInfo.fbId);
    if (!fb)
    {
        qWarning() << "Failed to get valid framebuffer info: " << strerror(errno);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(m_drmInfo.fd);
        m_drmInfo.fd = -1;
        return false;
    }

    // 6. 记录FB参数
    m_drmInfo.stride = fb->pitch;
    m_drmInfo.bpp    = fb->bpp;

    // 7. 映射有效FB到用户空间
    drm_mode_map_dumb mreq {};
    mreq.handle = fb->handle;
    if (drmIoctl(m_drmInfo.fd, DRM_IOCTL_MODE_MAP_DUMB, &mreq) < 0)
    {
        qWarning() << "Failed to map valid framebuffer: " << strerror(errno);
        drmModeFreeFB(fb);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(m_drmInfo.fd);
        m_drmInfo.fd = -1;
        return false;
    }

    // 8. 内存映射
    uint64_t fbSize = static_cast<uint64_t>(m_drmInfo.stride) * m_drmInfo.height;
    m_drmInfo.fbMap = mmap(nullptr, fbSize, PROT_READ | PROT_WRITE, MAP_SHARED, m_drmInfo.fd, mreq.offset);
    if (m_drmInfo.fbMap == MAP_FAILED)
    {
        qWarning() << "Failed to mmap framebuffer: " << strerror(errno);
        drmModeFreeFB(fb);
        drmModeFreeConnector(conn);
        drmModeFreeResources(res);
        close(m_drmInfo.fd);
        m_drmInfo.fd    = -1;
        m_drmInfo.fbMap = nullptr;
        return false;
    }

    // 9. 释放临时资源
    drmModeFreeFB(fb);
    if (conn)
        drmModeFreeConnector(conn);
    drmModeFreeResources(res);

    // 初始化成功
    m_drmInited = true;
    qInfo() << "DRM device initialized successfully with valid FB: " << m_drmInfo.width << "x" << m_drmInfo.height
            << " fbId=" << m_drmInfo.fbId;
    return true;
}

uint32_t ScreenShooter::findValidFbIdByResolution(int fd, int targetWidth, int targetHeight)
{
    if (fd < 0 || targetWidth <= 0 || targetHeight <= 0)
    {
        qWarning() << "Invalid parameters for finding FB";
        return 0;
    }

    qInfo() << "Searching FB for target resolution: " << targetWidth << "x" << targetHeight;

    // 1. 扩大FB ID遍历范围（覆盖更多场景）
    const uint32_t FB_ID_MAX         = 500;  // 从200扩大到500
    uint32_t       bestFbId          = 0;
    double         minResolutionDiff = 1000000.0;  // 初始化一个很大的差值

    for (uint32_t fbId = 1; fbId <= FB_ID_MAX; fbId++)
    {
        drmModeFB *fb = drmModeGetFB(fd, fbId);

        if (fb)
        {  // 该FB ID有效
            int fbWidth  = fb->width;
            int fbHeight = fb->height;
            qInfo() << "Check FB: id=" << fbId << " resolution=" << fbWidth << "x" << fbHeight;

            // 2. 计算分辨率相似度（差值越小越匹配）
            // 同时考虑宽、高的绝对差值，避免只匹配单维度
            double widthDiff  = abs(fbWidth - targetWidth);
            double heightDiff = abs(fbHeight - targetHeight);
            double totalDiff  = widthDiff + heightDiff;

            // 3. 优先选择差值最小的FB
            if (totalDiff < minResolutionDiff)
            {
                minResolutionDiff = totalDiff;
                bestFbId          = fbId;
            }

            drmModeFreeFB(fb);
        }
    }

    // 4. 校验：只有当差值在合理范围内（比如≤200）才认为有效
    if (bestFbId != 0 && minResolutionDiff <= 200)
    {
        // 打印选中的FB信息
        drmModeFB *bestFb = drmModeGetFB(fd, bestFbId);
        if (bestFb)
        {
            qInfo() << "Selected best match FB: id=" << bestFbId << " resolution=" << bestFb->width << "x"
                    << bestFb->height << " (diff: " << minResolutionDiff << ")";
            drmModeFreeFB(bestFb);
        }
        return bestFbId;
    }
    else
    {
        qWarning() << "No FB found in ID range [1-" << FB_ID_MAX << "] with reasonable resolution match (diff ≤ 200)";
        return 0;
    }
}

// ========== DRM资源清理 ==========
void ScreenShooter::cleanupDrmDevice()
{
    // 1. 解除内存映射
    if (m_drmInfo.fbMap)
    {
        uint64_t fbSize = static_cast<uint64_t>(m_drmInfo.stride) * m_drmInfo.height;
        munmap(m_drmInfo.fbMap, fbSize);
        m_drmInfo.fbMap = nullptr;
    }

    // 2. 销毁dumb framebuffer
    if (m_drmInfo.fbId && m_drmInfo.fd >= 0)
    {
        drm_mode_destroy_dumb dreq {};
        dreq.handle = m_drmInfo.fbId;
        drmIoctl(m_drmInfo.fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
        m_drmInfo.fbId = 0;
    }

    // 3. 关闭DRM设备
    if (m_drmInfo.fd >= 0)
    {
        if (m_drmInfo.fbMap != MAP_FAILED && m_drmInfo.fbMap != nullptr)
        {
            munmap(m_drmInfo.fbMap, static_cast<uint64_t>(m_drmInfo.stride) * m_drmInfo.height);
        }
        if (m_drmInfo.fbId != 0)
        {
            drm_mode_destroy_dumb dreq {};
            dreq.handle = m_drmInfo.fbId;
            drmIoctl(m_drmInfo.fd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
        }
        close(m_drmInfo.fd);
        m_drmInfo.fd = -1;
    }

    m_drmInited = false;
}

// ========== DRM截屏实现 ==========
QPixmap ScreenShooter::captureScreenDrm()
{
    if (!m_drmInited || !m_drmInfo.fbMap)
    {
        qWarning() << "DRM not initialized or framebuffer not mapped";
        return QPixmap();
    }

    // 关键：确认像素格式匹配（DRM默认是XRGB32，对应QImage的Format_RGB32）
    QImage::Format format = QImage::Format_RGB32;
    if (m_drmInfo.bpp == 32 && (m_drmInfo.stride / m_drmInfo.width) == 4)
    {
        format = QImage::Format_RGB32;  // XRGB32 (DRM) <-> RGB32 (QImage)
    }

    // 从GPU映射内存创建QImage
    QImage image(static_cast<uchar *>(m_drmInfo.fbMap), m_drmInfo.width, m_drmInfo.height, m_drmInfo.stride, format);

    // 拷贝数据避免内存悬空，转换为QPixmap
    return QPixmap::fromImage(image.copy());
}

// ========== X11截屏实现 ==========
QPixmap ScreenShooter::captureScreenX11()
{
    Display *display = XOpenDisplay(nullptr);
    if (!display)
    {
        qWarning() << "Failed to open X11 display";
        return QPixmap();
    }

    int               screen = DefaultScreen(display);
    Window            root   = RootWindow(display, screen);
    XWindowAttributes attrs {};
    if (!XGetWindowAttributes(display, root, &attrs))
    {
        qWarning() << "Failed to get X11 window attributes";
        XCloseDisplay(display);
        return QPixmap();
    }

    int width  = attrs.width;
    int height = attrs.height;

    // 创建X11共享内存图像
    XShmSegmentInfo shminfo {};
    XImage *ximage = XShmCreateImage(display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap,
                                     nullptr, &shminfo, width, height);

    if (!ximage)
    {
        qWarning() << "Failed to create XShm image";
        XCloseDisplay(display);
        return QPixmap();
    }

    // 分配共享内存
    shminfo.shmid = shmget(IPC_PRIVATE, ximage->bytes_per_line * ximage->height, IPC_CREAT | 0777);
    if (shminfo.shmid < 0)
    {
        qWarning() << "Failed to allocate XShm memory: " << strerror(errno);
        XDestroyImage(ximage);
        XCloseDisplay(display);
        return QPixmap();
    }

    // 附加共享内存
    shminfo.shmaddr = ximage->data = static_cast<char *>(shmat(shminfo.shmid, 0, 0));
    if (shminfo.shmaddr == reinterpret_cast<char *>(-1))
    {
        qWarning() << "Failed to attach XShm memory: " << strerror(errno);
        shmctl(shminfo.shmid, IPC_RMID, 0);
        XDestroyImage(ximage);
        XCloseDisplay(display);
        return QPixmap();
    }

    shminfo.readOnly = False;
    XShmAttach(display, &shminfo);

    // 捕获屏幕图像（共享内存方式，快速）
    XShmGetImage(display, root, ximage, 0, 0, AllPlanes);

    // ========== 核心修复：基于颜色掩码解析通道（彻底解决色彩问题） ==========
    // ========== 关键新增：获取X11的颜色掩码（适配所有环境） ==========
    Visual       *visual     = DefaultVisual(display, screen);
    unsigned long red_mask   = visual->red_mask;
    unsigned long green_mask = visual->green_mask;
    unsigned long blue_mask  = visual->blue_mask;

    // 计算每个颜色通道的偏移位（核心：不再硬编码通道位置）
    int red_shift = 0, green_shift = 0, blue_shift = 0;
    // 计算Red通道偏移
    while ((red_mask & 1) == 0)
    {
        red_shift++;
        red_mask >>= 1;
    }
    // 计算Green通道偏移
    while ((green_mask & 1) == 0)
    {
        green_shift++;
        green_mask >>= 1;
    }
    // 计算Blue通道偏移
    while ((blue_mask & 1) == 0)
    {
        blue_shift++;
        blue_mask >>= 1;
    }
    QImage          image(width, height, QImage::Format_RGB32);
    uchar          *dst       = image.bits();
    const uint32_t *src       = reinterpret_cast<const uint32_t *>(ximage->data);  // 按32位像素读取
    int             srcStride = ximage->bytes_per_line / 4;  // 每行像素数（而非字节数）

    for (int y = 0; y < height; ++y)
    {
        const uint32_t *srcRow = src + y * srcStride;
        uint32_t       *dstRow = reinterpret_cast<uint32_t *>(dst + y * image.bytesPerLine());

        for (int x = 0; x < width; ++x)
        {
            // 读取X11原始像素值
            uint32_t pixel = srcRow[x];

            // 基于掩码提取R/G/B通道（不再硬编码位置）
            uint8_t r = (pixel >> red_shift) & 0xFF;
            uint8_t g = (pixel >> green_shift) & 0xFF;
            uint8_t b = (pixel >> blue_shift) & 0xFF;

            // 写入Qt RGB32格式（A=255, R, G, B）
            // Qt的RGB32内存布局：0xAARRGGBB（小端存储为 BB GG RR AA）
            dstRow[x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
    QPixmap pixmap = QPixmap::fromImage(image);

    // 清理X11资源
    XShmDetach(display, &shminfo);
    shmdt(shminfo.shmaddr);
    shmctl(shminfo.shmid, IPC_RMID, 0);
    XDestroyImage(ximage);
    XCloseDisplay(display);

    // 更新屏幕分辨率
    m_screenWidth  = width;
    m_screenHeight = height;

    return pixmap;
}

// ========== 内部同步截屏实现（加锁保护） ==========
QPixmap ScreenShooter::captureScreenInternal()
{
    QMutexLocker locker(&m_captureMutex);  // 加锁保证线程安全

    // 缓存优化：50ms内的截图直接返回
    qint64 elapsed = m_lastCaptureTime.msecsTo(QDateTime::currentDateTime());
    if (elapsed < m_cacheTimeout && !m_lastFrame.isNull())
    {
        return m_lastFrame;
    }

    // 优先使用DRM截屏
    QPixmap pixmap;
    if (m_drmInited)
    {
        pixmap = captureScreenDrm();
        if (!pixmap.isNull())
        {
            m_lastFrame       = pixmap;
            m_lastCaptureTime = QDateTime::currentDateTime();
            return pixmap;
        }
        qWarning() << "DRM capture failed, fallback to X11";
    }

    // 回退到X11截屏
    pixmap            = captureScreenX11();
    m_lastFrame       = pixmap;
    m_lastCaptureTime = QDateTime::currentDateTime();

    return pixmap;
}

// ========== 同步截屏接口（对外兼容） ==========
QPixmap ScreenShooter::captureScreen()
{
    // 直接调用内部同步实现
    return captureScreenInternal();
}

// ========== 异步截屏接口（核心新增） ==========
std::future<QPixmap> ScreenShooter::captureScreenAsync()
{
    // 使用std::async启动异步任务，返回std::future
    // std::launch::async：强制在新线程执行
    return std::async(std::launch::async, [this]() -> QPixmap {
        // 调用加锁的内部截屏函数，保证线程安全
        return this->captureScreenInternal();
    });
}
