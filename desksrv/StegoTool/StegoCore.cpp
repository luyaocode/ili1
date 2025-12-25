#include "StegoCore.h"
#include <QFile>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QDebug>

// 定义大文件阈值（例如：100MB，可根据需求调整）
const qint64 LARGE_FILE_THRESHOLD = 100 * 1024 * 1024;  // 100MB

const int     SUFFIX_LEN      = 8;  // 定义文件后缀长度（8字节）
const int     DATA_LEN        = 4;  // 定义数据长度（4字节）
const int     BIT_COUNT_LEN   = 1;  // 定义LSB位数（1字节，取值范围1-8）
const int     HEADER_LEN      = SUFFIX_LEN + DATA_LEN + BIT_COUNT_LEN;  // 头部长度（13字节）
const int     HEADER_BITCOUNT = 1;                                      // 头部LSB位数
const QString SUFFIX_TEXT     = "txt";                                  // 文本默认后缀
const QString SUFFIX_BIN      = "bin";                                  // 二进制文件默认后缀

bool StegoCore::isImageSupported(const QImage &image)
{
    QImage::Format format = image.format();
    return format == QImage::Format_RGB32 ||               // Qt5.9 支持
           format == QImage::Format_ARGB32 ||              // Qt5.9 支持（替换 Format_ARGB8888）
           format == QImage::Format_RGB888 ||              // Qt5.9 支持
           format == QImage::Format_ARGB32_Premultiplied;  // Qt5.9 支持（预乘 Alpha 格式）
}

bool StegoCore::isBitCountSupported(const int bitcount)
{
    return bitcount > 0 && bitcount <= 8;
}

bool StegoCore::embed(const QImage                                         &image,
                      const QByteArray                                     &bits,
                      QImage                                               &outputImage,
                      const int                                             bitCount,
                      const std::function<void(float, float, std::string)> &update_progress,
                      const std::function<bool()>                          &is_cancelled)
{
    if (!isImageSupported(image))
        throw std::runtime_error("不支持的图片格式（仅支持RGB32/ARGB32/RGB888）");
    if (!isBitCountSupported(bitCount))
        throw std::runtime_error(QString("不支持的bitcount: %1").arg(bitCount).toStdString());
    qint64 maxSize = getMaxEmbedSize(image, bitCount);
    int    datalen = bits.size() / 8 - HEADER_LEN;
    if (datalen > maxSize)
        throw std::runtime_error(QString("嵌入数据过长：%1 字节，").arg(datalen).toStdString());
    if (bits.size() < 1)
        throw std::runtime_error(QString("嵌入数据为空").toStdString());

    outputImage = image.copy();
    // 统一转换为 Qt5.9 支持的 ARGB32 格式（兼容所有支持的输入格式）
    outputImage          = outputImage.convertToFormat(QImage::Format_ARGB32);
    int  width           = outputImage.width();
    int  height          = outputImage.height();
    int  bitIndex        = 0;
    int  totalBits       = bits.size();
    int  currBitCount    = HEADER_BITCOUNT;
    bool headerCompleted = false;
    for (int y = 0; y < height && bitIndex < totalBits; ++y)
    {
        for (int x = 0; x < width && bitIndex < totalBits; ++x)
        {
            QRgb pixel = outputImage.pixel(x, y);
            int  r     = qRed(pixel);
            int  g     = qGreen(pixel);
            int  b     = qBlue(pixel);

            // 修复：用指针数组遍历，循环变量改为 int* 类型（需修改通道值，必须用指针）
            int *channels[] = {&r, &g, &b};  // 指针数组，存储r/g/b的地址
            for (int *channel : channels)
            {  // 循环变量为int*，匹配指针类型
                if (bitIndex >= totalBits)
                {
                    break;
                }
                if (!headerCompleted && bitIndex >= HEADER_LEN * 8)
                {
                    currBitCount    = bitCount;
                    headerCompleted = true;
                }
                // 清除当前通道的最低bitCount位
                *channel &= ~((1 << currBitCount) - 1);
                // 嵌入bitCount位数据
                for (int i = 0; i < currBitCount && bitIndex < totalBits; ++i)
                {
                    *channel |= (bits[bitIndex] == '1') << i;
                    bitIndex++;
                    // 更新进度
                    if (update_progress && (bitIndex % int(totalBits * 0.01) == 0))
                    {
                        float pct = float(bitIndex) / totalBits;
                        update_progress(pct, pct, "");
                    }
                    // 任务取消
                    if (is_cancelled && is_cancelled())
                    {
                        return false;
                    }
                }
            }

            outputImage.setPixel(x, y, qRgba(r, g, b, qAlpha(pixel)));
        }
    }
    if (bitIndex < totalBits)
    {
        qWarning() << "图片没有足够空间嵌入数据";
    }
    return bitIndex == totalBits;
}

int StegoCore::extract(const QImage                                         &image,
                       QString                                              &suffix,
                       QByteArray                                           &data,
                       const int                                             bitCount,
                       const std::function<void(float, float, std::string)> &update_progress,
                       const std::function<bool()>                          &is_cancelled)
{
    if (!isImageSupported(image))
        throw std::runtime_error("不支持的图片格式（仅支持RGB/RGBA）");
    if (!isBitCountSupported(bitCount))
        throw std::runtime_error(QString("不支持的bitcount: %1").arg(bitCount).toStdString());

    QImage     tempImage = image.convertToFormat(QImage::Format_ARGB32);
    int        width     = tempImage.width();
    int        height    = tempImage.height();
    QByteArray bytes;
    int        byteBuffer      = 0;
    int        bitBufferCount  = 0;
    bool       headerCompleted = false;
    int        datalen         = 0;
    int        currBitCount    = HEADER_BITCOUNT;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            QRgb pixel = tempImage.pixel(x, y);
            int  r     = qRed(pixel);
            int  g     = qGreen(pixel);
            int  b     = qBlue(pixel);

            // 依次提取R、G、B通道的LSB位
            for (int channel : {r, g, b})
            {
                for (int i = 0; i < currBitCount; ++i)
                {
                    // 提取第i位并添加到字节缓冲区
                    byteBuffer = (byteBuffer << 1) | ((channel >> i) & 1);
                    bitBufferCount++;

                    // 缓冲区满8位，生成一个字节
                    if (bitBufferCount == 8)
                    {
                        bytes.append(static_cast<char>(byteBuffer));
                        byteBuffer     = 0;
                        bitBufferCount = 0;
                    }
                    if (!headerCompleted && bytes.size() == HEADER_LEN)
                    {
                        headerCompleted        = true;
                        QByteArray suffixBytes = bytes.left(SUFFIX_LEN);
                        suffix                 = strFromNArr(suffixBytes);

                        QByteArray lenBytes = bytes.mid(SUFFIX_LEN, DATA_LEN);
                        datalen             = intFromNArr(lenBytes);
                    }
                    if (headerCompleted && bytes.size() == HEADER_LEN + datalen)
                    {
                        data = bytes.mid(HEADER_LEN);
                        return datalen;
                    }
                    // 进度
                    if (headerCompleted && update_progress && bytes.size() % (int(datalen * 0.01)) == 0)
                    {
                        float pct = float(bytes.size()) / (datalen);
                        update_progress(pct, pct, "");
                    }
                    // 任务取消
                    if (is_cancelled && is_cancelled())
                    {
                        return false;
                    }
                }
                if (headerCompleted)
                {
                    currBitCount = bitCount;
                }
            }
        }
    }
    data = bytes.mid(HEADER_LEN);
    return datalen;
}

QByteArray StegoCore::toNArr(const QString &str, int N)
{
    // 边界：N≤0时返回空数组
    if (N <= 0)
    {
        return QByteArray();
    }
    // 步骤1：字符串转UTF-8字节数组（兼容多语言，如中文、英文、符号）
    QByteArray utf8Bytes = str.toUtf8();
    if (utf8Bytes.size() > N)
    {
        utf8Bytes.truncate(N);
    }
    // 情况2：字节数不足N，末尾补空格（ASCII 0x20）
    else if (utf8Bytes.size() < N)
    {
        utf8Bytes.append(QByteArray(N - utf8Bytes.size(), ' '));
    }
    // 情况3：长度正好为N，直接返回
    return utf8Bytes;
}

QString StegoCore::StegoCore::strFromNArr(const QByteArray &byteArr)
{
    // 步骤1：处理空数组边界
    if (byteArr.isEmpty())
    {
        return QString();
    }

    // 步骤2：剔除末尾补的空格（仅删除数组末尾的空格，保留字符串内的有效空格）
    QByteArray validBytes = byteArr;
    // 从后往前遍历，找到第一个非空格的位置，截断后续空格
    int lastValidIndex = validBytes.size() - 1;
    while (lastValidIndex >= 0 && validBytes[lastValidIndex] == ' ')
    {
        lastValidIndex--;
    }
    // 若全是空格，返回空字符串；否则截断到有效字节长度
    if (lastValidIndex < 0)
    {
        validBytes.clear();
    }
    else
    {
        validBytes.truncate(lastValidIndex + 1);
    }

    // 步骤3：UTF-8字节数组解码为QString（兼容多语言，容错无效UTF-8）
    // fromUtf8第二个参数：-1表示自动检测长度；第三个参数：无效字节替换为，避免解码失败
    QString result = QString::fromUtf8(validBytes);

    return result;
}

int StegoCore::intFromNArr(const QByteArray &byteArr)
{
    int       result = 0;
    const int N      = byteArr.size();  // 数组长度（即原函数的N）

    for (int i = 0; i < N; ++i)
    {
        // 跳过补位的空格（空格无数值意义）
        if (byteArr[i] == ' ')
            continue;

        // 小端序还原：第i字节 → 左移 i*8 位（低字节对应低位）
        uint8_t byteValue = static_cast<uint8_t>(byteArr[i]);
        // 先清空当前字节对应的位，再赋值（避免叠加脏数据）
        result &= ~(0xFF << (i * 8));
        result |= (byteValue << (i * 8));
    }

    return result;
}

// 核心实现：整数转16进制并规整为N字节数组
QByteArray StegoCore::toNArr(int n, int N)
{
    // 步骤1：初始化N字节的数组，默认填充空格（先满足“不足补空格”基础）
    QByteArray result(N, ' ');

    // 步骤2：按小端序将整数n写入数组（逐字节处理，低字节→高地址）
    for (int i = 0; i < N; ++i)
    {
        // 计算当前字节的偏移：i*8位
        const uint32_t shift = i * 8;
        // 提取当前字节的值（0xFF掩码确保只取8位）
        const uint8_t byteValue = static_cast<uint8_t>((n >> shift) & 0xFF);
        // 写入当前位置（小端序：第0位存最低字节，第1位存次低字节...）
        result[i] = static_cast<char>(byteValue);

        // 终止条件：若已到整数最高位，无需继续写入（避免无效移位）
        if (shift >= sizeof(int) * 8)
        {
            break;
        }
    }

    // 步骤3：超过N字节时自动截断（已通过循环i<N限制，无需额外处理）

    return result;
}

QByteArray StegoCore::bytesToBits(const QByteArray &bytes)
{
    QByteArray bits;
    for (char c : bytes)
    {
        for (int i = 7; i >= 0; --i)
        {
            bits.append((c >> i) & 1 ? '1' : '0');
        }
    }
    return bits;
}

qint64 StegoCore::getMaxEmbedSize(const QImage &image, const int bitCount)
{
    if (!isImageSupported(image))
        return 0;
    qint64 pixelCount   = image.width() * image.height();
    int    channelCount = 3;  // RGB通道写入数据，A通道不写入数据
    return (pixelCount * channelCount * bitCount) / 8 - HEADER_LEN;
}

bool StegoCore::extractHeader(const QImage &image, QString &suffix, int &len, int &bitCount)
{
    if (!isImageSupported(image))
    {
        return false;
    }

    QImage     tempImage = image.convertToFormat(QImage::Format_ARGB32);
    int        width     = tempImage.width();
    int        height    = tempImage.height();
    QByteArray bytes;
    int        byteBuffer     = 0;
    int        bitBufferCount = 0;
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            QRgb pixel = tempImage.pixel(x, y);
            int  r     = qRed(pixel);
            int  g     = qGreen(pixel);
            int  b     = qBlue(pixel);

            // 依次提取R、G、B通道的LSB位
            for (int channel : {r, g, b})
            {
                for (int i = 0; i < HEADER_BITCOUNT; ++i)
                {
                    // 提取第i位并添加到字节缓冲区
                    byteBuffer = (byteBuffer << 1) | ((channel >> i) & 1);
                    bitBufferCount++;

                    // 缓冲区满8位，生成一个字节
                    if (bitBufferCount == 8)
                    {
                        bytes.append(static_cast<char>(byteBuffer));
                        byteBuffer     = 0;
                        bitBufferCount = 0;
                    }
                    if (bytes.size() == HEADER_LEN)
                    {
                        QByteArray suffixBytes = bytes.left(SUFFIX_LEN);
                        suffix                 = strFromNArr(suffixBytes);

                        QByteArray lenBytes = bytes.mid(SUFFIX_LEN, DATA_LEN);
                        len                 = intFromNArr(lenBytes);

                        QByteArray bitCountBytes = bytes.mid(HEADER_LEN - 1, BIT_COUNT_LEN);
                        bitCount                 = intFromNArr(bitCountBytes);
                        return !suffix.isEmpty() && len > 0 && bitCount > 0 && bitCount <= 8;
                    }
                }
            }
        }
    }
    return false;
}

bool StegoCore::embedText(const QImage                                         &image,
                          const QString                                        &text,
                          QImage                                               &outputImage,
                          const std::function<void(float, float, std::string)> &update_progress,
                          const std::function<bool()>                          &is_cancelled,
                          const int                                             bitCount)
{
    QByteArray data   = text.toUtf8();
    QByteArray suffix = toNArr(SUFFIX_TEXT, SUFFIX_LEN);
    QByteArray len    = toNArr(data.size(), DATA_LEN);
    QByteArray bc     = toNArr(bitCount, BIT_COUNT_LEN);
    QByteArray bits   = bytesToBits(suffix + len + bc + data);
    return embed(image, bits, outputImage, bitCount, update_progress, is_cancelled);
}

bool StegoCore::extractText(const QImage                                         &image,
                            QString                                              &text,
                            const std::function<void(float, float, std::string)> &update_progress,
                            const std::function<bool()>                          &is_cancelled,
                            const int                                             bitCount)
{
    QString    suffix;
    QByteArray bytes;
    int        len = 0;
    int        bc  = bitCount;
    if (bitCount == 0)
    {
        bool succ = extractHeader(image, suffix, len, bc);
        if (!succ)
        {
            return false;
        }
    }
    if (suffix != SUFFIX_TEXT)
    {
        return false;
    }
    int size = extract(image, suffix, bytes, bc, update_progress, is_cancelled);
    Q_UNUSED(size)
    text = QString(bytes);
    return true;
}

bool StegoCore::embedBinary(const QImage                                         &image,
                            const QString                                        &path,
                            QImage                                               &outputImage,
                            const std::function<void(float, float, std::string)> &update_progress,
                            const std::function<bool()>                          &is_cancelled,
                            const int                                             bitCount)
{
    // 读取二进制文件数据
    QFile file(path);
    // 获取文件大小
    qint64 fileSize = file.size();
    if (fileSize > LARGE_FILE_THRESHOLD || fileSize > getMaxEmbedSize(image, bitCount))
    {
        throw std::runtime_error(QString("嵌入数据过长：%1 字节，").arg(fileSize).toStdString());
    }
    if (!file.exists() || !file.open(QIODevice::ReadOnly))
    {
        throw std::runtime_error(QString("打开文件失败：%1").arg(path).toStdString());
    }
    update_progress(0, 0, "Start reading binary data.");
    QByteArray data = file.readAll();
    file.close();

    QString strSuffix = QFileInfo(path).suffix();
    if (strSuffix.isEmpty())
    {
        strSuffix = SUFFIX_BIN;
    }
    QByteArray suffix = toNArr(strSuffix, SUFFIX_LEN);
    QByteArray len    = toNArr(data.size(), DATA_LEN);
    QByteArray bc     = toNArr(bitCount, BIT_COUNT_LEN);
    QByteArray bits   = bytesToBits(suffix + len + bc + data);
    return embed(image, bits, outputImage, bitCount, update_progress, is_cancelled);
}

bool StegoCore::extractBinary(const QImage                                         &image,
                              const QString                                        &outPath,
                              const std::function<void(float, float, std::string)> &update_progress,
                              const std::function<bool()>                          &is_cancelled,
                              const int                                             bitCount)
{
    if (outPath.isEmpty() || !QFileInfo(outPath).isDir() || !QFileInfo(outPath).exists())
    {
        update_progress(0, 0, "Save path is invalid.");
        return false;
    }
    int        bc  = bitCount;
    int        len = 0;
    QString    suffix;
    QByteArray bytes;
    if (bc == 0)
    {
        update_progress(0, 0, "Start extracting header.");
        bool succ = extractHeader(image, suffix, len, bc);
        if (!succ)
        {
            return false;
        }
    }
    update_progress(0, 0, "Start extracting binary file.");
    int size   = extract(image, suffix, bytes, bc, update_progress, is_cancelled);
    int bysize = bytes.size();
    if (size != bysize)
    {
        return false;
    }
    QString extractedSuffix = suffix;
    // 如果后缀为空，使用默认值
    if (extractedSuffix.isEmpty())
    {
        extractedSuffix = SUFFIX_BIN;
    }
    QString savePath = outPath + "/" + QString("extracted_file.%1").arg(extractedSuffix);
    // 写入文件
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Unbuffered))
    {
        update_progress(0, 0, "无法打开文件进行写入");
        return false;
    }

    qint64 bytesWritten = file.write(bytes);
    file.close();

    if (bytesWritten == -1)
    {
        update_progress(0, 0, "文件写入过程中发生错误！");
        return false;
    }
    else
    {
        update_progress(0, 0, "文件已成功保存");
    }
    return true;
}
