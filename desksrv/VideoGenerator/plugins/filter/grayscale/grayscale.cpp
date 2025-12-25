// filter_grayscale.cpp（编译为 libfilter_grayscale.so）
#include "plugin_interface.h"
#include <QImage>

EXTERNC_BEGIN

PluginInfo get_plugin_info()
{
    return {PluginType::Filter, "灰度滤镜", ""};
}

// 灰度处理：将彩色图转为灰度图
QImage create_filter(const QImage &src_img)
{
    if (src_img.isNull())
        return src_img;

    QImage dst_img(src_img.size(), QImage::Format_RGB888);
    for (int y = 0; y < dst_img.height(); y++)
    {
        for (int x = 0; x < dst_img.width(); x++)
        {
            QRgb pixel         = src_img.pixel(x, y);
            int  original_gray = qGray(pixel);

            // 对纯黑白像素添加灰度层次（示例：按坐标渐变）
            int gray;
            if (original_gray == 0)
            {  // 纯黑 → 按x坐标映射为0~127
                gray = qBound(0, (x * 127) / src_img.width(), 127);
            }
            else if (original_gray == 255)
            {  // 纯白 → 按y坐标映射为128~255
                gray = qBound(128, 128 + (y * 127) / src_img.height(), 255);
            }
            else
            {  // 已有灰度 → 保持或强化
                gray = original_gray;
            }

            dst_img.setPixel(x, y, qRgb(gray, gray, gray));
        }
    }
    return dst_img;
}

EXTERNC_END
