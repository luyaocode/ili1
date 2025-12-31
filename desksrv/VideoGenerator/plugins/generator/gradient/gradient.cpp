// generator_gradient.cpp（编译为 libgenerator_gradient.so）
#include "plugin_interface.h"
#include <QImage>
#include <QColor>
#include <QMediaRecorder>
#include <QCamera>
#include <QMediaPlayer>
#include <QFile>
#include <QFileInfo>
#include <QDir>

EXTERNC_BEGIN

PluginInfo get_plugin_info()
{
    return {PluginType::Generator, "纯色渐变生成器", "mp4"};
}

QImage create_frame_generator(int frame_index, int width, int height)
{
    QImage img(width, height, QImage::Format_RGB888);

    // 渐变偏移基准（由frame_index控制，避免渐变固定）
    int offset = frame_index % 256;

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // 1. 基于x坐标计算基础渐变值（0~255）
            int x_ratio = (x * 255) / width;  // x轴归一化到0-255

            // 2. 结合偏移计算RGB，实现动态渐变
            int r = qBound(0, 255 - (x_ratio + offset) % 256, 255);
            int b = qBound(0, (x_ratio + offset) % 256, 255);
            int g = qBound(0, 128 - abs(r - b), 255);

            img.setPixel(x, y, qRgb(r, g, b));
        }
    }
    return img;
}
// 对外暴露的视频生成函数
bool create_video_encoder(
    GenerateFrameFunc frame_func, int width, int height, int fps, int duration, const std::string &output_path)
{
    Q_UNUSED(frame_func)
    Q_UNUSED(width)
    Q_UNUSED(height)
    Q_UNUSED(fps)
    Q_UNUSED(duration)
    Q_UNUSED(output_path)
    return false;
}

EXTERNC_END
