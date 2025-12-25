#include "screenrecorder.h"
#include <QDateTime>
#include <QDebug>
#include <QMetaType>
#include <QTimer>
#include <QStandardPaths>
#include <QDir>
#include <opencv2/opencv.hpp>
#include "x11struct.h"
#include "tool.h"
// 声明元类型
Q_DECLARE_METATYPE(cv::Mat)

ScreenRecorder::ScreenRecorder(QObject *parent)
    : QObject(parent)
    , m_pX11Struct(std::shared_ptr<x11struct>(new x11struct(nullptr, 0)))
    , m_screenWidth(0)
    , m_screenHeight(0)
    , m_pVideoWriter(std::shared_ptr<cv::VideoWriter>(new cv::VideoWriter))
    , m_isRecording(false)
    , m_fps(15)
{
    // 初始化X11
    if (!initX11())
    {
        m_lastError = "Failed to initialize X11";
    }
    m_pCaptureTimer = new QTimer(this);
    // 连接定时器到捕获帧函数
    connect(m_pCaptureTimer, &QTimer::timeout, this, &ScreenRecorder::captureFrame);

    // 注册元类型
    qRegisterMetaType<cv::Mat>("cv::Mat");
    // 如果需要在队列中传递（跨线程），还需要注册常量引用版本
    qRegisterMetaType<cv::Mat>("cv::Mat&");
}

ScreenRecorder::~ScreenRecorder()
{
    // 确保停止录制
    if (m_isRecording)
    {
        stopRecording();
    }

    // 释放X11资源
    releaseX11();
}

bool ScreenRecorder::startRecording(const QString &fileName)
{
    QMutexLocker locker(&m_mutex);

    if (m_isRecording)
    {
        m_lastError = "Already recording";
        return false;
    }
    QString filePath = getSaveFilePath(fileName, ".mp4");

    // 检查X11是否初始化成功
    if (!m_pX11Struct->m_display || m_screenWidth <= 0 || m_screenHeight <= 0)
    {
        if (!initX11())
        {
            m_lastError = "X11 initialization failed";
            return false;
        }
    }

    // 打开视频写入器
    int fourcc = cv::VideoWriter::fourcc('a', 'v', 'c', '1');  // H.264编码
    m_pVideoWriter->open(filePath.toStdString(), fourcc, m_fps, cv::Size(m_screenWidth, m_screenHeight));

    if (!m_pVideoWriter->isOpened())
    {
        m_lastError = QString("Failed to open video file: %1").arg(filePath);
        return false;
    }

    // 启动定时器，控制帧率
    m_pCaptureTimer->start(1000 / m_fps);

    m_isRecording = true;
    emit recordingStateChanged(true);
    m_strFile = filePath;
    qDebug().noquote() << "[ScreenRecorder] 开始录制: " << m_strFile;
    return true;
}

void ScreenRecorder::stopRecording()
{
    QMutexLocker locker(&m_mutex);

    if (!m_isRecording)
    {
        return;
    }

    // 停止定时器
    bool bActive = m_pCaptureTimer->isActive();
    if (m_pCaptureTimer && bActive)
    {
        QMetaObject::invokeMethod(m_pCaptureTimer, "stop", Qt::AutoConnection);
    }

    // 释放视频写入器
    m_pVideoWriter->release();

    m_isRecording = false;
    emit recordingStateChanged(false);
    emit sigFileSaved(m_strFile);
    qDebug().noquote() << "[ScreenRecorder] Finished.";
}

void ScreenRecorder::screenShoot(const QString &fileName)
{
    cv::Mat frame;
    {
        QMutexLocker locker(&m_mutex);
        if (m_pX11Struct->m_display == nullptr)
        {
            initX11();
        }
        frame = captureScreenFrame();
    }
    QString filePath = getSaveFilePath(fileName, ".png");
    // 检查图像是否有效（非空）
    if (frame.empty())
    {
        qDebug() << "[ScreenRecorder] 保存失败：捕获的屏幕图像为空";
        return;
    }

    // 将 QString 转换为 OpenCV 支持的 std::string 路径
    std::string cpath = filePath.toStdString();

    // 使用 OpenCV 的 imwrite 保存图像
    // 支持的格式由文件后缀决定（如 .png、.jpg、.bmp 等）
    bool saveSuccess = cv::imwrite(cpath, frame);

    if (!saveSuccess)
    {
        qDebug() << "[ScreenRecorder] 保存失败：无法写入文件" << filePath << "，可能格式不支持或路径无效";
    }
    else
    {
        qDebug() << "[ScreenRecorder] 图像成功保存至：" << filePath;
    }
}

bool ScreenRecorder::isRecording() const
{
    QMutexLocker locker(&m_mutex);
    return m_isRecording;
}

QString ScreenRecorder::lastError() const
{
    QMutexLocker locker(&m_mutex);
    return m_lastError;
}

void ScreenRecorder::setFps(int fps)
{
    QMutexLocker locker(&m_mutex);
    if (fps > 0 && fps <= 60)
    {  // 限制帧率范围
        m_fps = fps;
        if (m_pCaptureTimer->isActive())
        {
            m_pCaptureTimer->setInterval(1000 / m_fps);
        }
    }
}

int ScreenRecorder::fps() const
{
    QMutexLocker locker(&m_mutex);
    return m_fps;
}

void ScreenRecorder::captureFrame()
{
    QMutexLocker locker(&m_mutex);
    if (!m_isRecording || !m_pX11Struct->m_display)
    {
        return;
    }

    // 捕获屏幕帧
    cv::Mat frame = captureScreenFrame();
    if (frame.empty())
    {
        m_lastError = "Failed to capture screen frame";
        //        stopRecording();
        // 创建与预期尺寸相同的黑屏帧
        // 假设预期尺寸为m_frameWidth x m_frameHeight，通道数为3（BGR）
        frame = cv::Mat::zeros(m_screenWidth, m_screenHeight, CV_8UC3);
    }
    // 绘制水印
    drawWatermark(frame, "desksrv");
    m_pVideoWriter->write(frame);
    // 发送帧可用信号（用于预览）
    emit frameAvailable(frame);
}

bool ScreenRecorder::initX11()
{
    // 打开显示
    m_pX11Struct->m_display = XOpenDisplay(nullptr);
    if (!m_pX11Struct->m_display)
    {
        m_lastError = "Failed to open X display";
        return false;
    }

    // 获取根窗口
    m_pX11Struct->m_rootWindow = DefaultRootWindow(m_pX11Struct->m_display);

    // 获取屏幕属性
    XWindowAttributes gwa;
    if (!XGetWindowAttributes(m_pX11Struct->m_display, m_pX11Struct->m_rootWindow, &gwa))
    {
        m_lastError = "Failed to get window attributes";
        XCloseDisplay(m_pX11Struct->m_display);
        m_pX11Struct->m_display = nullptr;
        return false;
    }

    m_screenWidth  = gwa.width;
    m_screenHeight = gwa.height;

    return true;
}

void ScreenRecorder::releaseX11()
{
    if (m_pX11Struct->m_display)
    {
        XCloseDisplay(m_pX11Struct->m_display);
        m_pX11Struct->m_display = nullptr;
    }
    m_pX11Struct->m_rootWindow = 0;
    m_screenWidth              = 0;
    m_screenHeight             = 0;
}

cv::Mat ScreenRecorder::captureScreenFrame()
{
    if (!m_pX11Struct->m_display || m_screenWidth <= 0 || m_screenHeight <= 0)
    {
        return cv::Mat();
    }

    // 获取屏幕图像
    XImage *ximage = XGetImage(m_pX11Struct->m_display, m_pX11Struct->m_rootWindow, 0, 0, m_screenWidth, m_screenHeight,
                               AllPlanes, ZPixmap);

    if (!ximage)
    {
        m_lastError = "Failed to get X image";
        return cv::Mat();
    }

    // 创建OpenCV矩阵（BGR格式）
    cv::Mat frame(m_screenHeight, m_screenWidth, CV_8UC3);

    // 转换X11图像格式到OpenCV格式（BGR）
    for (int y = 0; y < m_screenHeight; ++y)
    {
        for (int x = 0; x < m_screenWidth; ++x)
        {
            // 计算像素位置
            uint32_t pixel = ((uint32_t *)ximage->data)[y * m_screenWidth + x];

            uint8_t red   = 0;
            uint8_t green = 0;
            uint8_t blue  = 0;
            if (ximage->bits_per_pixel == 32)
            {
                if (ximage->red_mask == 0x00ff0000)
                {  // XRGB格式
                    red   = (pixel >> 16) & 0xFF;
                    green = (pixel >> 8) & 0xFF;
                    blue  = pixel & 0xFF;
                }
                else if (ximage->red_mask == 0x0000ff)
                {  // BGRX格式
                    red   = pixel & 0xFF;
                    green = (pixel >> 8) & 0xFF;
                    blue  = (pixel >> 16) & 0xFF;
                }
            }

            // 设置到OpenCV矩阵
            frame.at<cv::Vec3b>(y, x) = cv::Vec3b(blue, green, red);
        }
    }

    // 释放X11图像
    XDestroyImage(ximage);

    return frame;
}
