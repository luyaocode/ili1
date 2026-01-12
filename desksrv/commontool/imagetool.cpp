#include "imagetool.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <QBuffer>
#include <QImageWriter>

cv::Mat qImageToMat(const QImage &image)
{
    cv::Mat mat;
    switch (image.format())
    {
        case QImage::Format_RGB888:
            mat = cv::Mat(image.height(), image.width(), CV_8UC3, (void *)image.bits(), image.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);  // OpenCV默认BGR格式
            break;
        case QImage::Format_ARGB32:
        case QImage::Format_RGBA8888:
            mat = cv::Mat(image.height(), image.width(), CV_8UC4, (void *)image.bits(), image.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_RGBA2BGR);
            break;
        case QImage::Format_Grayscale8:
            mat = cv::Mat(image.height(), image.width(), CV_8UC1, (void *)image.bits(), image.bytesPerLine());
            break;
        default:
            // 转换为RGB888兼容格式
            QImage img = image.convertToFormat(QImage::Format_RGB888);
            mat        = cv::Mat(img.height(), img.width(), CV_8UC3, (void *)img.bits(), img.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
            break;
    }
    return mat;
}

QImage matToQImage(const cv::Mat &mat)
{
    QImage image;
    if (mat.type() == CV_8UC3)
    {
        cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
        image = QImage((const uchar *)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
    }
    else if (mat.type() == CV_8UC4)
    {
        cv::cvtColor(mat, mat, cv::COLOR_BGRA2RGBA);
        image = QImage((const uchar *)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_RGBA8888);
    }
    else if (mat.type() == CV_8UC1)
    {
        image = QImage((const uchar *)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
    }
    else
    {
        // 转换为RGB888兼容格式
        cv::Mat temp;
        mat.convertTo(temp, CV_8UC3);
        cv::cvtColor(temp, temp, cv::COLOR_BGR2RGB);
        image = QImage((const uchar *)temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
    }
    return image.copy();  // 深拷贝避免内存问题
}

QImage enhanceImageQuality(const QImage &srcImage)
{
    if (srcImage.isNull())
    {
        return srcImage;  // 空图片直接返回
    }

    // 1. 转换为OpenCV Mat
    cv::Mat srcMat = qImageToMat(srcImage);
    cv::Mat enhancedMat;
    srcMat.copyTo(enhancedMat);

    // ========== 调整为中度增强参数（降低亮度/锐化/对比度） ==========
    const double sharpenAlpha   = 1.2;  // 锐化强度（从2.0降至1.2，大幅降低锐化）
    const double contrastAlpha  = 1.15; // 对比度强度（从1.4降至1.15，小幅降低）
    const double brightnessBeta = 5;    // 亮度补偿（从15降至5，大幅降低亮度增益）
    const int    denoiseH       = 5;    // 去噪强度保持不变（不影响亮度）

    // 2. 第一步：去噪（轻度去噪，优先保留细节）
    cv::fastNlMeansDenoisingColored(enhancedMat, enhancedMat, denoiseH, denoiseH * 2, 7, 21);

    // 3. 第二步：降低强度的对比度+亮度增强
    enhancedMat.convertTo(enhancedMat, -1, contrastAlpha, brightnessBeta);

    // 4. 第三步：中度锐化（减弱边缘强化）
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 0, -1, 0, -1, 5, -1,  // 核强度从6降回5，降低锐化基础强度
                      0, -1, 0);
    cv::filter2D(enhancedMat, enhancedMat, -1, kernel * sharpenAlpha);

    // 5. 第四步：伽马校正（从重度提亮改为轻度，降低暗部提亮）
    double  gamma = 0.9;  // 从0.8升至0.9（值越大越接近原图亮度，0.9仅轻度提亮）
    cv::Mat gammaMat;
    enhancedMat.convertTo(gammaMat, CV_32F, 1.0 / 255.0);
    cv::pow(gammaMat, gamma, gammaMat);
    gammaMat.convertTo(enhancedMat, CV_8UC3, 255.0);

    // 6. 额外：饱和度增强（降低饱和度提升幅度）
    cv::Mat hsvMat;
    cv::cvtColor(enhancedMat, hsvMat, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> hsvChannels;
    cv::split(hsvMat, hsvChannels);
    hsvChannels[1] = hsvChannels[1] * 1.1;  // 饱和度从+30%降至+10%，减少色彩过艳导致的视觉亮度
    cv::merge(hsvChannels, hsvMat);
    cv::cvtColor(hsvMat, enhancedMat, cv::COLOR_HSV2BGR);

    // 7. 转换回QImage
    return matToQImage(enhancedMat);
}
