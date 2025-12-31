// generator_text.cpp（编译为 libgenerator_text.so）
#include "plugin_interface.h"
#include <QImageWriter>
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QBuffer>
#include <QMovie>
#include <QByteArray>
#include <QFile>

EXTERNC_BEGIN
PluginInfo get_plugin_info()
{
    return {PluginType::Generator, "闪烁文字生成器", "gif"};
}

// 闪烁文字生成：文字透明度随帧索引变化
QImage create_frame_generator(int frame_index, int width, int height)
{
    QImage img(width, height, QImage::Format_ARGB32);
    img.fill(Qt::black);

    QPainter painter(&img);
    painter.setPen(QColor(255, 255, 255));
    painter.setFont(QFont("Arial", 48, QFont::Bold));
    painter.setRenderHint(QPainter::TextAntialiasing);

    // 透明度闪烁：0-255 循环
    int alpha = abs(255 - (frame_index % 510));
    painter.setPen(QColor(255, 255, 255, alpha));
    painter.drawText(img.rect(), Qt::AlignCenter, "视频生成器");

    return img;
}

// GIF 编码导出（基于 QImageWriter）
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
