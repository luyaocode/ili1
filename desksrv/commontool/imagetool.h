#ifndef IMAGETOOL_H
#define IMAGETOOL_H
#include <opencv2/opencv.hpp>
#include <QImage>
#include <QPixmap>
#include <QByteArray>
#include <QRect>
#include <QPoint>
// 高清级别枚举
enum HdLevel
{
    HdLevel_JpegHigh = 0,  // JPEG高质量（95%）
    HdLevel_JpegLossless,  // JPEG无损（100%+无采样）
    HdLevel_PngLossless    // PNG无损
};

/**
 * @brief 远程桌面常用特效枚举
 * @details 采用位掩码(2的幂次)设计，支持任意特效叠加组合，使用 | 组合，& 判断
 */
enum SpecialEffect
{
    Effect_None            = 0x0000,  // 0 - 无特效，基础值
    Effect_Grayscale       = 0x0001,  // 1 - 灰度化：弱网降带宽/色盲友好，最常用叠加项
    Effect_CompressQuality = 0x0002,  // 2 - 画质压缩：按比例降低画质，减小传输体积，无损分辨率
    Effect_Sharpen         = 0x0004,  // 4 - 锐化增强：远程桌面文字/线条模糊专用，提升清晰度
    Effect_GaussianBlur    = 0x0008,  // 8 - 高斯模糊：隐私遮挡/降低细节减少传输量
    Effect_InvertColor     = 0x0010,  // 16 - 反色显示：应急适配（比如屏幕花屏/色偏）
    Effect_Brightness      = 0x0020,  // 32 - 亮度调节：适配不同端的显示亮度差异
    Effect_Contrast        = 0x0040,  // 64 - 对比度调节：远程桌面文字/图像分层更清晰
    Effect_MouseHighlight = 0x0080,  // 128 - 鼠标光标高亮：远程协作时，对方光标位置一眼识别（刚需）
    Effect_ReduceNoise      = 0x0100,  // 256 - 降噪滤波：采集端屏幕噪点/水波纹消除，画面更干净
    Effect_MirrorHorizontal = 0x0200,  // 512 - 水平镜像：应急适配（比如采集画面左右颠倒）
    Effect_MirrorVertical   = 0x0400   // 1024- 垂直镜像：同上
};
// 辅助函数：Qt QImage 转 OpenCV Mat（保留）
cv::Mat qImageToMat(const QImage &image);

// 辅助函数：OpenCV Mat 转 Qt QImage（保留）
QImage matToQImage(const cv::Mat &mat);

// 图像增强
QImage enhanceImageQuality(const QImage &srcImage);

// 差异区域计算
QRect calculateDiffRect(const QPixmap &prev, const QPixmap &curr, int threshold);

// 编码高清图像
QByteArray encodeHdImage(const QImage &image, HdLevel level);

////////////////////////////// 特效////////////////////////////

/**
 * @brief 灰度化处理，直接修改原图像
 * @param img 待处理的图像，输入输出为同一张图
 * @return bool 处理成功返回true，失败返回false
 */
bool applyGrayscale(QImage &img);

/**
 * @brief 图像锐化处理，直接修改原图像，增强边缘细节
 * @param img 待处理的图像，输入输出为同一张图
 * @return bool 处理成功返回true，失败返回false
 */
bool applySharpen(QImage &img);

/**
 * @brief 高斯模糊处理，直接修改原图像，柔和模糊效果
 * @param img 待处理的图像，输入输出为同一张图
 * @return bool 处理成功返回true，失败返回false
 */
bool applyGaussianBlur(QImage &img);

/**
 * @brief 图像颜色反相处理，直接修改原图像
 * @param img 待处理的图像，输入输出为同一张图
 * @return bool 处理成功返回true，失败返回false
 */
bool applyInvertColor(QImage &img);

/**
 * @brief 亮度调节处理，直接修改原图像
 * @param img 待处理的图像，输入输出为同一张图
 * @param delta 亮度值，范围[-100,100]，正数提亮，负数变暗，默认+20
 * @return bool 处理成功返回true，失败返回false
 */
bool applyBrightness(QImage &img, int delta = 20);

/**
 * @brief 对比度调节处理，直接修改原图像
 * @param img 待处理的图像，输入输出为同一张图
 * @param factor 对比度系数，范围[0.0,3.0]，1.0为原图，默认1.2
 * @return bool 处理成功返回true，失败返回false
 */
bool applyContrast(QImage &img, qreal factor = 1.2);

/**
 * @brief 鼠标光标高亮处理，绘制黄色圆环+十字线，直接修改原图像
 * @param img 待处理的图像，输入输出为同一张图
 * @param pos 鼠标在图像中的坐标点
 * @param radius 高亮圆环半径，默认15像素，最小5像素
 * @return bool 处理成功返回true，失败返回false
 */
bool applyMouseHighlight(QImage &img, const QPoint &pos, int radius = 15);

/**
 * @brief 图像降噪处理，中值滤波消除噪点，保留细节，直接修改原图像
 * @param img 待处理的图像，输入输出为同一张图
 * @return bool 处理成功返回true，失败返回false
 */
bool applyReduceNoise(QImage &img);

/**
 * @brief 图像水平镜像翻转，直接修改原图像
 * @param img 待处理的图像，输入输出为同一张图
 * @return bool 处理成功返回true，失败返回false
 */
bool applyMirrorHorizontal(QImage &img);

/**
 * @brief 图像垂直镜像翻转，直接修改原图像
 * @param img 待处理的图像，输入输出为同一张图
 * @return bool 处理成功返回true，失败返回false
 */
bool applyMirrorVertical(QImage &img);

/**
 * @brief 图像画质压缩处理，降低画质减小体积，无损分辨率，直接修改原图像
 * @param img 待处理的图像，输入输出为同一张图
 * @param quality 压缩质量，范围[1,100]，值越高画质越好，默认70
 * @return bool 处理成功返回true，失败返回false
 */
bool applyCompressQuality(QImage &img, int quality = 70);

/**
 * @brief 批量叠加特效处理，根据传入的特效组合自动依次处理
 * @param img 待处理的图像，输入输出为同一张图
 * @param effects 特效组合，支持多特效位或组合，如Effect_Grayscale|Effect_Sharpen
 * @param mousePos 鼠标坐标，仅鼠标高亮特效生效，默认原点
 * @return bool 全部特效处理成功返回true，任一失败返回false
 */
bool applySpecialEffects(QImage &img, int effects, const QPoint &mousePos = QPoint(0,0));

#endif  // IMAGETOOL_H
