// generator_grid.cpp（编译为 libgenerator_grid.so）
#include "plugin_interface.h"
#include <QImage>
#include <QPainter>
#include <QFile>

EXTERNC_BEGIN

PluginInfo get_plugin_info()
{
    return {PluginType::Generator, "网格动画生成器", "avi"};
}

// AVI 编码导出（模拟）
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
// 导出接口
QImage create_frame_generator(int frame_index, int width, int height)
{
    QImage img(width, height, QImage::Format_RGB888);
    img.fill(Qt::white);

    QPainter painter(&img);
    painter.setPen(QColor(0, 0, 0, 100));  // 半透明黑色网格
    int grid_size = 40;
    int offset    = frame_index % grid_size;  // 网格偏移

    // 绘制水平网格线
    for (int y = offset; y < height; y += grid_size)
    {
        painter.drawLine(0, y, width, y);
    }
    // 绘制垂直网格线
    for (int x = offset; x < width; x += grid_size)
    {
        painter.drawLine(x, 0, x, height);
    }

    return img;
}

EXTERNC_END
