#ifndef IMAGEDETECTOR_H
#define IMAGEDETECTOR_H

#include <QObject>
#include <QImage>
#include <opencv2/opencv.hpp>
#include <vector>
#include <cmath>

// 兼容 C++11 的 clamp 实现（模板版，支持任意数值类型）
template<typename T>
constexpr T std_clamp(const T& value, const T& min_val, const T& max_val) {
    // C++11 三元运算符实现核心逻辑
    return (value < min_val) ? min_val : ((value > max_val) ? max_val : value);
}

// 检测结果结构体
struct DetectResult
{
    bool  isAIGenerated;  // 是否AI生成
    float score;          // 综合评分（0~1，越高越可能是AI生成）
    // 分项特征值（用于调试）
    float noiseRandomScore;     // 噪声随机性得分（0~1，AI图趋近1）
    float gradientAbnormScore;  // 梯度异常得分（0~1，AI图趋近1）
    float freqRegularScore;     // 频域规律得分（0~1，AI图趋近1）
};

class ImageDetector : public QObject
{
    Q_OBJECT
public:
    explicit ImageDetector(QObject *parent = nullptr);

    // 核心检测接口
    DetectResult detect(const QImage &image);

private:
    // ========== 特征提取函数 ==========
    // 1. 噪声随机性检测（AI图噪声更规律，得分更高）
    float calcNoiseRandomness(const cv::Mat &grayMat);

    // 2. 像素梯度分布检测（AI图梯度异常，得分更高）
    float calcGradientAbnormality(const cv::Mat &grayMat);

    // 3. 频域特征检测（AI图高频分量更规律，得分更高）
    float calcFreqRegularity(const cv::Mat &grayMat);

    // ========== 辅助函数 ==========
    // 将任意QImage转换为标准RGBA8888格式（避免格式兼容问题）
    QImage convertToStandardFormat(const QImage &image);
    // QImage转OpenCV灰度图
    cv::Mat qimage2GrayMat(const QImage &image);

    // 阈值配置（可根据样本调优）
    const float NOISE_WEIGHT    = 0.35f;  // 噪声特征权重
    const float GRADIENT_WEIGHT = 0.35f;  // 梯度特征权重
    const float FREQ_WEIGHT     = 0.30f;  // 频域特征权重
    const float THRESHOLD       = 0.5f;   // 综合评分阈值（超过则判定为AI生成）
};

#endif  // IMAGEDETECTOR_H
