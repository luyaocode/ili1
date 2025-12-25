#ifndef STEGOCORE_H
#define STEGOCORE_H

#include <QImage>
#include <QString>
#include <QByteArray>
#include <stdexcept>
#include <functional>

class StegoCore
{
public:
    // 嵌入文本到图片
    static bool embedText(const QImage                                         &image,
                          const QString                                        &text,
                          QImage                                               &outputImage,
                          const std::function<void(float, float, std::string)> &update_progress = nullptr,
                          const std::function<bool()>                          &is_cancelled    = nullptr,
                          const int                                             bitCount        = 1);

    // 从图片提取文本
    static bool extractText(const QImage                                         &image,
                            QString                                              &text,
                            const std::function<void(float, float, std::string)> &update_progress = nullptr,
                            const std::function<bool()>                          &is_cancelled    = nullptr,
                            const int                                             bitCount        = 0);

    // 嵌入文件到图片
    static bool embedBinary(const QImage                                         &image,
                            const QString                                        &path,
                            QImage                                               &outputImage,
                            const std::function<void(float, float, std::string)> &update_progress = nullptr,
                            const std::function<bool()>                          &is_cancelled    = nullptr,
                            const int                                             bitCount        = 1);

    // 从图片提取文件
    static bool extractBinary(const QImage                                         &image,
                              const QString                                        &outPath,
                              const std::function<void(float, float, std::string)> &update_progress = nullptr,
                              const std::function<bool()>                          &is_cancelled    = nullptr,
                              const int                                             bitCount        = 0);

    // 计算最大可嵌入字节数
    static qint64 getMaxEmbedSize(const QImage &image, const int bitCount = 1);

    // 解析数据头部
    static bool extractHeader(const QImage &image, QString &suffix, int &len, int &bitCount);

private:
    // 字节序列转二进制流
    static QByteArray bytesToBits(const QByteArray &bytes);

    // 检查图片是否支持隐写（必须是 RGB/RGBA 格式）
    static bool isImageSupported(const QImage &image);

    // 检查图片是否支持隐写（必须是 RGB/RGBA 格式）
    static bool isBitCountSupported(const int bitcount);

    // 写入数据
    static bool embed(const QImage                                         &image,
                      const QByteArray                                     &bits,
                      QImage                                               &outputImage,
                      const int                                             bitCount,
                      const std::function<void(float, float, std::string)> &update_progress,
                      const std::function<bool()>                          &is_cancelled);

    static int extract(const QImage                                         &image,
                       QString                                              &suffix,
                       QByteArray                                           &data,
                       const int                                             bitCount,
                       const std::function<void(float, float, std::string)> &update_progress,
                       const std::function<bool()>                          &is_cancelled);
    // 字符串规整为N字节数组，不足的用空格补齐，超过的截断
    static QByteArray toNArr(const QString &str, int N);
    // 逆运算函数：N字节UTF-8数组 → QString（剔除补的空格，还原原始字符串）
    static QString strFromNArr(const QByteArray &byteArr);
    // 16进制规整为N字节数组，不足的用空格补齐，超过的截断
    static QByteArray toNArr(int n, int N);
    // 逆运算函数：N字节小端序数组 → int
    static int intFromNArr(const QByteArray &byteArr);
};

#endif  // STEGOCORE_H
