#include "ClientInfo.h"

QString getHdLevelString(HdLevel level)
{
    switch (level)
    {
        case HdLevel::HdLevel_JpegHigh:
            return "jepg_high";
        case HdLevel::HdLevel_JpegLossless:
            return "jepg_lossless";
        case HdLevel::HdLevel_PngLossless:
            return "png_lossless";
        default:
            return "";
    }
    return "";
}

HdLevel getHdLevelFromString(const QString &level)
{
    // 先统一转为小写，避免大小写不一致问题（如JPEG_HIGH、Jpeg_High等）
    QString levelLower = level.trimmed().toLower();

    // 字符串映射到枚举值
    if (levelLower == "jpeg_high")
    {
        return HdLevel::HdLevel_JpegHigh;
    }
    else if (levelLower == "jpeg_lossless")
    {
        return HdLevel::HdLevel_JpegLossless;
    }
    else if (levelLower == "png_lossless")
    {
        return HdLevel::HdLevel_PngLossless;
    }
    else
    {
        qWarning() << "Invalid HdLevel string:" << level << ", use default PNG lossless";
        return HdLevel::HdLevel_PngLossless;
    }
}
