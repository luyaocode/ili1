#ifndef SCREENRECORDER_H
#define SCREENRECORDER_H

#include <QTextStream>
#include <QObject>
#include <QString>
#include <QMutex>
#include <memory>

// forward declaration
class QTimer;
namespace cv
{
    class Mat;
    class VideoWriter;
}

struct x11struct;
class ScreenRecorder : public QObject
{
    Q_OBJECT
public:
    explicit ScreenRecorder(QObject *parent = nullptr);
    ~ScreenRecorder();

    // 开始录制，返回是否成功
    bool startRecording(const QString &outputPath = QString());

    // 停止录制
    void stopRecording();

    // 获取当前录制状态
    bool isRecording() const;

    // 获取最后一次错误信息
    QString lastError() const;

    // 设置录制帧率
    void setFps(int fps);

    // 获取录制帧率
    int fps() const;
signals:
    // 录制状态变化信号
    void recordingStateChanged(bool isRecording);

    // 新的视频帧可用（用于预览）
    void frameAvailable(const cv::Mat &frame);

private slots:
    // 捕获一帧屏幕
    void captureFrame();

private:
    // 初始化X11
    bool initX11();

    // 释放X11资源
    void releaseX11();

    // 捕获屏幕帧并转换为OpenCV格式
    cv::Mat captureScreenFrame();
private:
    std::shared_ptr<x11struct> m_pX11Struct;
    int m_screenWidth;
    int m_screenHeight;
    std::shared_ptr<cv::VideoWriter> m_pVideoWriter; // OpenCV视频写入器
    QTimer* m_pCaptureTimer = nullptr; // 定时器控制帧率
    bool m_isRecording; // 录制状态
    int m_fps; // 帧率
    QString m_lastError; // 错误信息
    mutable QMutex m_mutex; // 互斥锁
};

#endif // SCREENRECORDER_H

