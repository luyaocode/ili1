#include "imagedetector.h"
#include <QDebug>
#include <numeric>

ImageDetector::ImageDetector(QObject *parent): QObject(parent)
{
}

// 辅助函数：计算向量方差
float calcVariance(const std::vector<float>& vec) {
    if (vec.empty()) return 0.0f;
    float mean = std::accumulate(vec.begin(), vec.end(), 0.0f) / vec.size();
    float variance = 0.0f;
    for (float v : vec) {
        variance += (v - mean) * (v - mean);
    }
    return variance / vec.size();
}

// 辅助函数：计算向量熵
float calcEntropy(const std::vector<float>& vec) {
    if (vec.empty()) return 0.0f;

    // 统计值的分布（分箱）
    const int binNum = 64;
    float minVal = *std::min_element(vec.begin(), vec.end());
    float maxVal = *std::max_element(vec.begin(), vec.end());
    float binWidth = (maxVal - minVal) / binNum + 1e-6;

    std::vector<int> bins(binNum, 0);
    for (float v : vec) {
        int binIdx = std::min(static_cast<int>((v - minVal) / binWidth), binNum - 1);
        bins[binIdx]++;
    }

    // 计算熵
    float total = static_cast<float>(vec.size());
    float entropy = 0.0f;
    for (int count : bins) {
        if (count == 0) continue;
        float p = count / total;
        entropy -= p * log2(p);
    }
    return entropy;
}

// 辅助函数：计算向量偏度（衡量分布不对称性）
float calcSkewness(const std::vector<float>& vec) {
    if (vec.size() < 2) return 0.0f;
    float mean = std::accumulate(vec.begin(), vec.end(), 0.0f) / vec.size();
    float var = calcVariance(vec);
    if (var < 1e-6) return 0.0f;

    float skew = 0.0f;
    for (float v : vec) {
        skew += pow(v - mean, 3);
    }
    skew /= (vec.size() * pow(var, 1.5));
    return skew;
}

// 辅助函数：计算向量分位数
float calcQuantile(std::vector<float> vec, float quantile) {
    if (vec.empty()) return 0.0f;
    std::sort(vec.begin(), vec.end());
    int idx = static_cast<int>(vec.size() * quantile);
    idx = std_clamp(idx, 0, static_cast<int>(vec.size()) - 1);
    return vec[idx];
}
// 转换为标准RGBA8888格式（兜底所有格式）
QImage ImageDetector::convertToStandardFormat(const QImage &image)
{
    // 如果已是标准格式，直接返回（避免冗余拷贝）
    if (image.format() == QImage::Format_RGBA8888 || image.format() == QImage::Format_RGB888)
    {
        return image;
    }

    // 对所有其他格式，统一转换为RGBA8888（覆盖所有QImage支持的格式）
    // QImage::convertToFormat 会自动处理不同位深、色彩空间的转换
    QImage standardImage = image.convertToFormat(QImage::Format_RGBA8888);
    if (standardImage.isNull())
    {
        qCritical() << "QImage格式转换失败：无法转换为RGBA8888格式";
    }
    return standardImage;
}
// 主转换函数：支持所有QImage格式
cv::Mat ImageDetector::qimage2GrayMat(const QImage &image)
{
    // 空图片直接返回空Mat
    if (image.isNull())
    {
        qWarning() << "输入QImage为空";
        return cv::Mat();
    }

    // 第一步：将任意格式转换为标准RGBA8888（兜底所有格式）
    QImage standardImage = convertToStandardFormat(image);
    if (standardImage.isNull())
    {
        return cv::Mat();
    }

    // 第二步：根据标准格式转换为OpenCV Mat并转灰度图
    cv::Mat mat;
    switch (standardImage.format())
    {
        case QImage::Format_RGB888:
        {
            // RGB888 -> OpenCV BGR（QImage是RGB，OpenCV默认BGR）-> 灰度
            mat = cv::Mat(standardImage.height(), standardImage.width(), CV_8UC3, (void *)standardImage.bits(),
                          standardImage.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_RGB2GRAY);  // 注意QImage的RGB顺序
            break;
        }
        case QImage::Format_RGBA8888:
        {
            // RGBA8888 -> 灰度（忽略Alpha通道）
            mat = cv::Mat(standardImage.height(), standardImage.width(), CV_8UC4, (void *)standardImage.bits(),
                          standardImage.bytesPerLine());
            cv::cvtColor(mat, mat, cv::COLOR_RGBA2GRAY);
            break;
        }
        default:
        {
            // 理论上不会走到这里（已提前转换为标准格式）
            qWarning() << "未预期的标准格式：" << standardImage.format();
            return cv::Mat();
        }
    }

    // 重要：OpenCV Mat默认共享QImage的内存，这里拷贝一份避免内存悬空
    cv::Mat grayMat = mat.clone();
    return grayMat;
}

// 核心检测逻辑
DetectResult ImageDetector::detect(const QImage &image)
{
    DetectResult result;
    result.isAIGenerated = false;
    result.score         = 0.0f;

    // 1. 转换为灰度图（简化特征提取）
    cv::Mat grayMat = qimage2GrayMat(image);
    if (grayMat.empty())
    {
        qCritical() << "图片转换失败";
        return result;
    }

    // 2. 提取三类特征
    float noiseScore    = calcNoiseRandomness(grayMat);      // 噪声规律得分（0~1）
    float gradientScore = calcGradientAbnormality(grayMat);  // 梯度异常得分（0~1）
    float freqScore     = calcFreqRegularity(grayMat);       // 频域规律得分（0~1）

    // 3. 加权计算综合得分
    result.score = noiseScore * NOISE_WEIGHT + gradientScore * GRADIENT_WEIGHT + freqScore * FREQ_WEIGHT;

    // 4. 阈值判断
    result.isAIGenerated = (result.score >= THRESHOLD);

    // 5. 填充分项得分（调试用）
    result.noiseRandomScore    = noiseScore;
    result.gradientAbnormScore = gradientScore;
    result.freqRegularScore    = freqScore;

    return result;
}

// 修正1：噪声随机性检测（增加噪声分布的偏度特征）
float ImageDetector::calcNoiseRandomness(const cv::Mat &grayMat)
{
    if (grayMat.empty() || grayMat.type() != CV_8UC1)
    {
        qWarning() << "calcNoiseRandomness: 输入非有效灰度图";
        return 0.0f;
    }

    // 步骤1：优化噪声提取（双边滤波+形态学去伪影）
    cv::Mat blurMat;
    cv::bilateralFilter(grayMat, blurMat, 9, 75, 75);
    cv::Mat noiseMat;
    cv::absdiff(grayMat, blurMat, noiseMat);

    // 步骤2：自适应分块（基于图像尺寸动态调整）
    int BLOCK_SIZE = std::min(32, std::min(grayMat.rows / 8, grayMat.cols / 8));
    BLOCK_SIZE     = std::max(BLOCK_SIZE, 8);
    int rows       = (grayMat.rows + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int cols       = (grayMat.cols + BLOCK_SIZE - 1) / BLOCK_SIZE;

    std::vector<float> blockEntropies;
    std::vector<float> blockSkewness;  // 新增：噪声分布偏度
    for (int i = 0; i < rows; ++i)
    {
        for (int j = 0; j < cols; ++j)
        {
            int      x = j * BLOCK_SIZE;
            int      y = i * BLOCK_SIZE;
            int      w = std::min(BLOCK_SIZE, grayMat.cols - x);
            int      h = std::min(BLOCK_SIZE, grayMat.rows - y);
            cv::Rect roi(x, y, w, h);
            cv::Mat  block = noiseMat(roi);

            // 提取块内噪声值
            std::vector<float> noiseVals;
            for (int r = 0; r < block.rows; ++r)
            {
                const uchar *ptr = block.ptr<uchar>(r);
                for (int c = 0; c < block.cols; ++c)
                {
                    if (ptr[c] > 0)
                        noiseVals.push_back(static_cast<float>(ptr[c]));
                }
            }

            // 计算熵和偏度
            float entropy = calcEntropy(noiseVals);
            float skew    = calcSkewness(noiseVals);
            blockEntropies.push_back(entropy);
            blockSkewness.push_back(std::abs(skew));  // 偏度绝对值（AI噪声分布更对称）
        }
    }

    // 步骤3：融合熵和偏度特征，自适应归一化
    float avgEntropy = std::accumulate(blockEntropies.begin(), blockEntropies.end(), 0.0f) / blockEntropies.size();
    float avgSkew    = std::accumulate(blockSkewness.begin(), blockSkewness.end(), 0.0f) / blockSkewness.size();

    // 真实图：熵高、偏度高；AI图：熵低、偏度低
    float maxEntropy  = log2(128);                         // 直方图维度对应的最大熵
    float normEntropy = 1.0f - (avgEntropy / maxEntropy);  // 熵越低（AI），得分越高
    float normSkew    = 1.0f - (avgSkew / 2.0f);           // 偏度越低（AI），得分越高（2为经验上限）

    // 加权融合（熵权重0.7，偏度0.3）
    float finalScore = (normEntropy * 0.7 + normSkew * 0.3);
    return std_clamp(finalScore, 0.0f, 1.0f);
}

// 修正2：像素梯度异常检测（分位数阈值+偏度特征）
float ImageDetector::calcGradientAbnormality(const cv::Mat &grayMat)
{
    if (grayMat.empty() || grayMat.type() != CV_8UC1)
    {
        qWarning() << "calcGradientAbnormality: 输入非有效灰度图";
        return 0.0f;
    }

    // 步骤1：Scharr梯度计算
    cv::Mat gradX, gradY;
    cv::Scharr(grayMat, gradX, CV_32F, 1, 0);
    cv::Scharr(grayMat, gradY, CV_32F, 0, 1);

    // 步骤2：梯度幅值（归一化+去异常值）
    cv::Mat gradMag;
    cv::magnitude(gradX, gradY, gradMag);
    cv::normalize(gradMag, gradMag, 0, 255, cv::NORM_MINMAX);

    // 提取梯度值并过滤异常值（95分位数以内）
    std::vector<float> gradValues;
    for (int i = 0; i < gradMag.rows; ++i)
    {
        const float *rowPtr = gradMag.ptr<float>(i);
        for (int j = 0; j < gradMag.cols; ++j)
        {
            gradValues.push_back(rowPtr[j]);
        }
    }
    float              q95 = calcQuantile(gradValues, 0.95);
    std::vector<float> validGrads;
    for (float v : gradValues)
    {
        if (v <= q95)
            validGrads.push_back(v);
    }

    if (validGrads.empty())
        return 0.0f;

    // 步骤3：提取梯度特征（方差+偏度）
    float gradVar  = calcVariance(validGrads);
    float gradSkew = calcSkewness(validGrads);

    // 步骤4：自适应阈值（基于真实/AI图的统计规律）
    // 真实图：梯度方差大（>1000）、偏度高（>0.5）；AI图：方差小（<500）、偏度低（<0.2）
    float varScore = gradVar < 500 ? 1.0f : (gradVar > 1000 ? 0.0f : (1000 - gradVar) / 500);
    float skewScore =
        std::abs(gradSkew) < 0.2 ? 1.0f : (std::abs(gradSkew) > 0.5 ? 0.0f : (0.5 - std::abs(gradSkew)) / 0.3);

    // 加权融合（方差0.6，偏度0.4）
    float finalScore = (varScore * 0.6 + skewScore * 0.4);
    return std_clamp(finalScore, 0.0f, 1.0f);
}

// 修正3：频域特征检测（高频能量占比+动态熵阈值）
float ImageDetector::calcFreqRegularity(const cv::Mat &grayMat)
{
    if (grayMat.empty() || grayMat.type() != CV_8UC1)
    {
        qWarning() << "calcFreqRegularity: 输入非有效灰度图";
        return 0.0f;
    }

    // 步骤1：FFT最优尺寸扩展
    int     optRows = cv::getOptimalDFTSize(grayMat.rows);
    int     optCols = cv::getOptimalDFTSize(grayMat.cols);
    cv::Mat padded;
    cv::copyMakeBorder(grayMat, padded, 0, optRows - grayMat.rows, 0, optCols - grayMat.cols, cv::BORDER_REPLICATE);

    // 步骤2：FFT计算
    cv::Mat planes[] = {cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F)};
    cv::Mat complexMat;
    cv::merge(planes, 2, complexMat);
    cv::dft(complexMat, complexMat);

    // 步骤3：幅度谱计算+中心化
    cv::split(complexMat, planes);
    cv::magnitude(planes[0], planes[1], planes[0]);
    cv::Mat magMat = planes[0];
    magMat += cv::Scalar::all(1);
    cv::log(magMat, magMat);

    // 中心化（低频移到中心）
    cv::Mat magCentered = magMat.clone();
    int     cx          = magCentered.cols / 2;
    int     cy          = magCentered.rows / 2;
    cv::Mat q1(magCentered, cv::Rect(0, 0, cx, cy));
    cv::Mat q2(magCentered, cv::Rect(cx, 0, cx, cy));
    cv::Mat q3(magCentered, cv::Rect(0, cy, cx, cy));
    cv::Mat q4(magCentered, cv::Rect(cx, cy, cx, cy));

    cv::Mat tmp;
    q1.copyTo(tmp);
    q4.copyTo(q1);
    tmp.copyTo(q4);
    q2.copyTo(tmp);
    q3.copyTo(q2);
    tmp.copyTo(q3);

    // 步骤4：高频区域划分+能量占比计算
    // 高频区域：距中心>1/4图像尺寸的区域
    int                highFreqRadius = std::min(cx, cy) / 4;
    std::vector<float> highFreqVals;
    float              totalEnergy    = 0.0f;
    float              highFreqEnergy = 0.0f;

    for (int i = 0; i < magCentered.rows; ++i)
    {
        const float *rowPtr = magCentered.ptr<float>(i);
        for (int j = 0; j < magCentered.cols; ++j)
        {
            float val = rowPtr[j];
            totalEnergy += val;

            // 计算到中心的距离
            float dx   = j - cx;
            float dy   = i - cy;
            float dist = sqrt(dx * dx + dy * dy);
            if (dist > highFreqRadius)
            {
                highFreqVals.push_back(val);
                highFreqEnergy += val;
            }
        }
    }

    if (highFreqVals.empty() || totalEnergy < 1e-6)
        return 0.0f;

    // 步骤5：高频能量占比 + 高频熵
    float highFreqRatio = highFreqEnergy / totalEnergy;  // 真实图：高频占比高（>0.15）；AI图：占比低（<0.08）
    float highFreqEntropy = calcEntropy(highFreqVals);
    float maxEntropy      = log2(highFreqVals.size());
    float normEntropy     = 1.0f - (highFreqEntropy / maxEntropy);  // AI图熵低，得分高

    // 步骤6：自适应得分计算
    // 能量占比得分：AI图占比低→得分高
    float ratioScore = highFreqRatio < 0.08 ? 1.0f : (highFreqRatio > 0.15 ? 0.0f : (0.15 - highFreqRatio) / 0.07);
    // 熵得分 + 能量占比得分融合（各0.5权重）
    float finalScore = (normEntropy * 0.5 + ratioScore * 0.5);
    return std_clamp(finalScore, 0.0f, 1.0f);
}
