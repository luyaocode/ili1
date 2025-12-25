// plugin_interface.h（更新后）
#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <QImage>
#include <string>
#define EXTERNC_BEGIN \
    extern "C"        \
    {
#define EXTERNC_END }

enum class PluginType
{
    Generator,
    Filter,
    PluginTypes
};
struct PluginInfo
{
    PluginType  type;
    std::string name;
    std::string extra;
};

typedef PluginInfo (*PluginInfoFunc)();
// 帧生成函数类型：输入帧索引、分辨率，输出生成的单帧图像
typedef QImage (*GenerateFrameFunc)(int frame_index, int width, int height);
// 视频编码导出函数：输入帧生成函数、参数，导出为目标文件
typedef bool (*EncodeVideoFunc)(
    GenerateFrameFunc frame_func, int width, int height, int fps, int duration, const std::string &output_path);
typedef QImage (*FilterFunc)(const QImage &src_img);

EXTERNC_BEGIN
// --------------- 通用插件接口---------------
PluginInfo get_plugin_info();

// --------------- 生成器插件接口---------------
// 创建帧生成器
QImage create_frame_generator(int frame_index, int width, int height);

// 创建视频编码器
bool create_video_encoder(
    GenerateFrameFunc frame_func, int width, int height, int fps, int duration, const std::string &output_path);

// --------------- 滤镜插件接口---------------
// 滤镜
QImage create_filter(const QImage &src_img);

EXTERNC_END

#endif  // PLUGIN_INTERFACE_H
