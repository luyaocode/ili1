// filter_blur.cpp（编译为 libfilter_blur.so）
#include "plugin_interface.h"
#include <QImage>
#include <QPainter>
#include <QGraphicsBlurEffect>  // 添加头文件
#include <QGraphicsScene>       // 添加头文件（实现模糊的必要类）
#include <QGraphicsPixmapItem>  // 添加头文件

EXTERNC_BEGIN

PluginInfo get_plugin_info()
{
    return {PluginType::Filter, "模糊滤镜", ""};
}

// 模糊处理：简化为均值模糊
QImage create_filter(const QImage &src_img)
{
    if (src_img.isNull())
        return src_img;

    int    width  = src_img.width();
    int    height = src_img.height();
    QImage dst_img(width, height, QImage::Format_RGB888);

    // 模糊核大小（奇数，越大模糊越强：3/5/7/9，推荐7）
    const int kernel_size   = 7;
    const int kernel_radius = kernel_size / 2;
//    const int kernel_area   = kernel_size * kernel_size;  // 核面积（用于均值计算）

    // 遍历每个像素，计算邻域均值
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int sum_r = 0, sum_g = 0, sum_b = 0;
            int count = 0;

            // 遍历卷积核邻域
            for (int ky = -kernel_radius; ky <= kernel_radius; ky++)
            {
                for (int kx = -kernel_radius; kx <= kernel_radius; kx++)
                {
                    // 边界保护：避免越界
                    int nx = x + kx;
                    int ny = y + ky;
                    if (nx >= 0 && nx < width && ny >= 0 && ny < height)
                    {
                        QRgb pixel = src_img.pixel(nx, ny);
                        sum_r += qRed(pixel);
                        sum_g += qGreen(pixel);
                        sum_b += qBlue(pixel);
                        count++;
                    }
                }
            }

            // 计算均值并设置像素
            int r = sum_r / count;
            int g = sum_g / count;
            int b = sum_b / count;
            dst_img.setPixel(x, y, qRgb(r, g, b));
        }
    }

    return dst_img;
}

EXTERNC_END
