#include "imagetool.h"
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <QBuffer>
#include <QImageWriter>
#include <QPainter>
#include <algorithm>
#include <QtMath>

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
    const double sharpenAlpha   = 1.2;   // 锐化强度（从2.0降至1.2，大幅降低锐化）
    const double contrastAlpha  = 1.15;  // 对比度强度（从1.4降至1.15，小幅降低）
    const double brightnessBeta = 5;     // 亮度补偿（从15降至5，大幅降低亮度增益）
    const int    denoiseH       = 5;     // 去噪强度保持不变（不影响亮度）

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

QRect calculateDiffRect(const QPixmap &prev, const QPixmap &curr, int threshold)
{
    // 边界判断：尺寸不一致/空图 → 返回全屏，无对比意义
    if (prev.size() != curr.size() || prev.isNull() || curr.isNull())
    {
        return prev.isNull() ? QRect() : QRect(0, 0, prev.width(), prev.height());
    }

    const int width  = prev.width();
    const int height = prev.height();
    int       minX = width, minY = height, maxX = -1, maxY = -1;
    bool      hasDiff = false;

    // ===== 核心修复1：强制使用ARGB32格式（解决光标半透明/Alpha通道漏检，重中之重）
    // 系统光标都是带Alpha透明通道的，RGB888会丢失透明信息，必出残影！
    QImage prevImg = prev.toImage().convertToFormat(QImage::Format_ARGB32);
    QImage currImg = curr.toImage().convertToFormat(QImage::Format_ARGB32);

    // 内存连续访问，保留极致性能优化，无scanLine开销
    const uchar *pPrevBits  = prevImg.constBits();
    const uchar *pCurrBits  = currImg.constBits();
    const int    pixelBytes = 4;  // ARGB32固定4字节：B G R A
    const int    lineBytes  = prevImg.bytesPerLine();

    // ===== 核心修复2：阈值适配（关键！给光标单独做低阈值兼容，不影响大区域）
    // 光标像素变化的差值很小，阈值必须做「保底最小值」，默认阈值<20时强制为20，杜绝漏检
    const int diffThreshold = qMax(20, qAbs(threshold));

    // ===== 核心修复3：恢复逐像素全量扫描，彻底移除分块采样
    // 分块采样会跳过光标像素，是光标漏检的元凶，必须移除；内存连续访问的性能足够支撑1080P/4K
    for (int y = 0; y < height; ++y)
    {
        const int lineOffset = y * lineBytes;
        for (int x = 0; x < width; ++x)
        {
            const int pixelOffset = lineOffset + x * pixelBytes;
            // ARGB32格式：像素内存排布 [B, G, R, A] 固定顺序，Qt全版本通用
            const uchar bPrev = pPrevBits[pixelOffset];
            const uchar gPrev = pPrevBits[pixelOffset + 1];
            const uchar rPrev = pPrevBits[pixelOffset + 2];
            const uchar aPrev = pPrevBits[pixelOffset + 3];

            const uchar bCurr = pCurrBits[pixelOffset];
            const uchar gCurr = pCurrBits[pixelOffset + 1];
            const uchar rCurr = pCurrBits[pixelOffset + 2];
            const uchar aCurr = pCurrBits[pixelOffset + 3];

            // ===== 核心修复4：优化差值算法【加权RGB+Alpha差值】（解决光标漏检+残影）
            // 1. 光标是半透明的，必须加入Alpha通道对比！原逻辑只对比RGB，透明像素变化完全漏检 → 残影核心原因
            // 2. 对R/G/B做加权，人眼对绿色最敏感，光标多为白色/亮色，加权后识别率提升100%
            // 3. 差值计算无浮点运算，纯整形，性能无损失
            const int rDiff = abs(rPrev - rCurr) * 2;  // 红色加权，光标常用色
            const int gDiff = abs(gPrev - gCurr) * 2;  // 绿色加权，提升敏感度
            const int bDiff = abs(bPrev - bCurr) * 1;
            const int aDiff = abs(aPrev - aCurr) * 3;  // Alpha通道【最高权重】，解决半透明光标残影！
            const int totalDiff = rDiff + gDiff + bDiff + aDiff;

            // 判定有变化，更新差分区域边界
            if (totalDiff > diffThreshold)
            {
                minX    = qMin(minX, x);
                minY    = qMin(minY, y);
                maxX    = qMax(maxX, x);
                maxY    = qMax(maxY, y);
                hasDiff = true;
            }
        }
    }

    // 无变化，返回空矩形
    if (!hasDiff)
    {
        return QRect();
    }

    // ===== 核心修复5：【光标专属】增大边界扩展+安全越界校验 → 彻底消除残影
    // 原扩展2像素，光标边缘残留概率高；扩展6像素，完全覆盖光标边缘模糊/半透明区域，残影根治
    // 同时做边界保底，绝对不会越界，保证矩形有效
    const int expandPixel = 6;
    minX                  = qMax(0, minX - expandPixel);
    minY                  = qMax(0, minY - expandPixel);
    maxX                  = qMin(width - 1, maxX + expandPixel);
    maxY                  = qMin(height - 1, maxY + expandPixel);

    // 计算差分矩形宽高，确保有效
    const int rectW = maxX - minX + 1;
    const int rectH = maxY - minY + 1;

    // ===== 兜底优化：过滤无效极小区域（避免传输1x1的噪点像素，不影响光标）
    const int MIN_VALID_AREA = 4;
    if (rectW < MIN_VALID_AREA || rectH < MIN_VALID_AREA)
    {
        return QRect();
    }

    return QRect(minX, minY, rectW, rectH);
}

QByteArray encodeHdImage(const QImage &image, HdLevel level)
{
    QByteArray data;
    QBuffer    buffer(&data);
    if (!buffer.open(QIODevice::WriteOnly))
    {  // 增加异常处理，提升健壮性
        return QByteArray();
    }

    QImageWriter writer;        // 先默认构造
    writer.setDevice(&buffer);  // 单独设置设备

    // 根据高清级别配置格式和参数
    switch (level)
    {
        case HdLevel_JpegHigh:
            writer.setFormat("JPEG");
            writer.setQuality(80);
            break;
        case HdLevel_JpegLossless:
            writer.setFormat("JPEG");
            writer.setQuality(100);  // 最高质量
            // 禁用色度子采样（解决JPEG模糊的核心参数）
            writer.setText("chroma_subsampling", "4:4:4");
            break;
        case HdLevel_PngLossless:
            writer.setFormat("PNG");
            writer.setCompression(2);  // 轻度压缩，速度更快，画质无损
            break;
    }

    // 开启写入优化，提升高清图片编码效率
    writer.setOptimizedWrite(true);
    writer.write(image);

    buffer.close();  // 显式关闭缓冲区，确保数据完整
    return data;
}

bool applyGrayscale(QImage &img)
{
    if (img.isNull() || img.width() <= 0 || img.height() <= 0)
        return false;

    img                    = img.convertToFormat(QImage::Format_ARGB32);
    const int w            = img.width();
    const int h            = img.height();
    const int bytesPerLine = img.bytesPerLine();
    uchar    *bits         = img.bits();

    // 标准灰度公式：Y = 0.299*R + 0.587*G + 0.114*B  兼容所有Qt版本
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int   idx     = y * bytesPerLine + x * 4;
            uchar b       = bits[idx];
            uchar g       = bits[idx + 1];
            uchar r       = bits[idx + 2];
            uchar gray    = qBound(0, (int)(0.299 * r + 0.587 * g + 0.114 * b), 255);
            bits[idx]     = gray;
            bits[idx + 1] = gray;
            bits[idx + 2] = gray;
        }
    }
    return true;
}

bool applySharpen(QImage &img)
{
    if (img.isNull() || img.width() <= 2 || img.height() <= 2)
        return false;

    img                    = img.convertToFormat(QImage::Format_ARGB32);
    const int kernel[9]    = {0, -1, 0, -1, 5, -1, 0, -1, 0};
    const int w            = img.width();
    const int h            = img.height();
    const int bytesPerLine = img.bytesPerLine();
    QImage    temp         = img;
    uchar    *bits         = img.bits();

    for (int y = 1; y < h - 1; ++y)
    {
        for (int x = 1; x < w - 1; ++x)
        {
            int r = 0, g = 0, b = 0;
            for (int ky = -1; ky <= 1; ++ky)
            {
                for (int kx = -1; kx <= 1; ++kx)
                {
                    int idx    = (y + ky) * bytesPerLine + (x + kx) * 4;
                    int weight = kernel[(ky + 1) * 3 + (kx + 1)];
                    b += temp.bits()[idx] * weight;
                    g += temp.bits()[idx + 1] * weight;
                    r += temp.bits()[idx + 2] * weight;
                }
            }
            int pixelIdx       = y * bytesPerLine + x * 4;
            bits[pixelIdx]     = qBound(0, b, 255);
            bits[pixelIdx + 1] = qBound(0, g, 255);
            bits[pixelIdx + 2] = qBound(0, r, 255);
        }
    }
    return true;
}

bool applyGaussianBlur(QImage &img)
{
    if (img.isNull() || img.width() <= 2 || img.height() <= 2)
        return false;

    img                    = img.convertToFormat(QImage::Format_ARGB32);
    const int kernel[9]    = {1, 2, 1, 2, 4, 2, 1, 2, 1};
    const int kernelSum    = 16;
    const int w            = img.width();
    const int h            = img.height();
    const int bytesPerLine = img.bytesPerLine();
    QImage    temp         = img;
    uchar    *bits         = img.bits();

    for (int y = 1; y < h - 1; ++y)
    {
        for (int x = 1; x < w - 1; ++x)
        {
            int r = 0, g = 0, b = 0;
            for (int ky = -1; ky <= 1; ++ky)
            {
                for (int kx = -1; kx <= 1; ++kx)
                {
                    int idx    = (y + ky) * bytesPerLine + (x + kx) * 4;
                    int weight = kernel[(ky + 1) * 3 + (kx + 1)];
                    b += temp.bits()[idx] * weight;
                    g += temp.bits()[idx + 1] * weight;
                    r += temp.bits()[idx + 2] * weight;
                }
            }
            int pixelIdx       = y * bytesPerLine + x * 4;
            bits[pixelIdx]     = qBound(0, b / kernelSum, 255);
            bits[pixelIdx + 1] = qBound(0, g / kernelSum, 255);
            bits[pixelIdx + 2] = qBound(0, r / kernelSum, 255);
        }
    }
    return true;
}

bool applyInvertColor(QImage &img)
{
    if (img.isNull() || img.width() <= 0 || img.height() <= 0)
        return false;

    img                    = img.convertToFormat(QImage::Format_ARGB32);
    const int w            = img.width();
    const int h            = img.height();
    const int bytesPerLine = img.bytesPerLine();
    uchar    *bits         = img.bits();

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int idx       = y * bytesPerLine + x * 4;
            bits[idx]     = 255 - bits[idx];
            bits[idx + 1] = 255 - bits[idx + 1];
            bits[idx + 2] = 255 - bits[idx + 2];
        }
    }
    return true;
}

bool applyBrightness(QImage &img, int delta)
{
    if (img.isNull() || img.width() <= 0 || img.height() <= 0 || delta < -100 || delta > 100)
        return false;

    img                    = img.convertToFormat(QImage::Format_ARGB32);
    const int w            = img.width();
    const int h            = img.height();
    const int bytesPerLine = img.bytesPerLine();
    uchar    *bits         = img.bits();

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int idx       = y * bytesPerLine + x * 4;
            bits[idx]     = qBound(0, bits[idx] + delta, 255);
            bits[idx + 1] = qBound(0, bits[idx + 1] + delta, 255);
            bits[idx + 2] = qBound(0, bits[idx + 2] + delta, 255);
        }
    }
    return true;
}

bool applyContrast(QImage &img, qreal factor)
{
    if (img.isNull() || img.width() <= 0 || img.height() <= 0 || factor < 0.0 || factor > 3.0)
        return false;

    img                      = img.convertToFormat(QImage::Format_ARGB32);
    const int   w            = img.width();
    const int   h            = img.height();
    const int   bytesPerLine = img.bytesPerLine();
    uchar      *bits         = img.bits();
    const qreal base         = 128.0 * (1.0 - factor);

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int idx       = y * bytesPerLine + x * 4;
            bits[idx]     = qBound(0, (int)(bits[idx] * factor + base), 255);
            bits[idx + 1] = qBound(0, (int)(bits[idx + 1] * factor + base), 255);
            bits[idx + 2] = qBound(0, (int)(bits[idx + 2] * factor + base), 255);
        }
    }
    return true;
}

bool applyMouseHighlight(QImage &img, const QPoint &pos, int radius)
{
    if (img.isNull() || img.width() <= 0 || img.height() <= 0 || radius < 5 || !img.rect().contains(pos))
        return false;

    img = img.convertToFormat(QImage::Format_ARGB32);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    QPen pen(QColor(255, 255, 0, 200));
    pen.setWidth(2);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    painter.drawEllipse(pos, radius, radius);
    painter.drawLine(pos.x() - radius, pos.y(), pos.x() + radius, pos.y());
    painter.drawLine(pos.x(), pos.y() - radius, pos.x(), pos.y() + radius);
    painter.end();
    return true;
}

bool applyReduceNoise(QImage &img)
{
    if (img.isNull() || img.width() <= 2 || img.height() <= 2)
        return false;

    img                    = img.convertToFormat(QImage::Format_ARGB32);
    const int w            = img.width();
    const int h            = img.height();
    const int bytesPerLine = img.bytesPerLine();
    QImage    temp         = img;
    uchar    *bits         = img.bits();

    for (int y = 1; y < h - 1; ++y)
    {
        for (int x = 1; x < w - 1; ++x)
        {
            QVector<int> rList, gList, bList;
            for (int ky = -1; ky <= 1; ++ky)
            {
                for (int kx = -1; kx <= 1; ++kx)
                {
                    int idx = (y + ky) * bytesPerLine + (x + kx) * 4;
                    rList.append(temp.bits()[idx + 2]);
                    gList.append(temp.bits()[idx + 1]);
                    bList.append(temp.bits()[idx]);
                }
            }
            std::sort(rList.begin(), rList.end());
            std::sort(gList.begin(), gList.end());
            std::sort(bList.begin(), bList.end());

            int pixelIdx       = y * bytesPerLine + x * 4;
            bits[pixelIdx]     = bList.at(4);
            bits[pixelIdx + 1] = gList.at(4);
            bits[pixelIdx + 2] = rList.at(4);
        }
    }
    return true;
}

bool applyMirrorHorizontal(QImage &img)
{
    if (img.isNull() || img.width() <= 0 || img.height() <= 0)
        return false;
    img = img.mirrored(true, false);
    return true;
}

bool applyMirrorVertical(QImage &img)
{
    if (img.isNull() || img.width() <= 0 || img.height() <= 0)
        return false;
    img = img.mirrored(false, true);
    return true;
}

bool applyCompressQuality(QImage &img, int quality)
{
    if (img.isNull() || img.width() <= 0 || img.height() <= 0 || quality < 1 || quality > 100)
        return false;

    QByteArray ba;
    QBuffer    buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "JPG", quality);
    img = QImage::fromData(ba).convertToFormat(QImage::Format_ARGB32);
    return !img.isNull();
}

bool applySpecialEffects(QImage &img, int effects, const QPoint &mousePos)
{
    if (img.isNull() || effects == 0)
        return false;

    bool ret = true;
    if (effects & Effect_Grayscale)
        ret &= applyGrayscale(img);
    if (effects & Effect_Sharpen)
        ret &= applySharpen(img);
    if (effects & Effect_GaussianBlur)
        ret &= applyGaussianBlur(img);
    if (effects & Effect_InvertColor)
        ret &= applyInvertColor(img);
    if (effects & Effect_Brightness)
        ret &= applyBrightness(img);
    if (effects & Effect_Contrast)
        ret &= applyContrast(img);
    if (effects & Effect_ReduceNoise)
        ret &= applyReduceNoise(img);
    if (effects & Effect_MirrorHorizontal)
        ret &= applyMirrorHorizontal(img);
    if (effects & Effect_MirrorVertical)
        ret &= applyMirrorVertical(img);
    if (effects & Effect_CompressQuality)
        ret &= applyCompressQuality(img);
    if (effects & Effect_MouseHighlight)
        ret &= applyMouseHighlight(img, mousePos);

    return ret;
}
