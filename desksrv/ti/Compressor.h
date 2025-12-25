// 压缩/解压缩工具
#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include <string>

class Compressor {
public:
    // 压缩文件
    // inputPath: 输入文件路径
    // outputPath: 输出文件路径
    // 返回值: true 表示成功, false 表示失败
    static bool compress(const std::string& inputPath, const std::string& outputPath);

    // 解压文件
    // inputPath: 输入文件路径 (LZH 格式)
    // outputPath: 输出文件路径
    // 返回值: true 表示成功, false 表示失败
    static bool decompress(const std::string& inputPath, const std::string& outputPath);

private:
    // 禁止实例化
    Compressor() = delete;
    ~Compressor() = delete;
private:
    static bool compressText(const std::string& inputPath, const std::string& outputPath);
    static bool compressBinary(const std::string& inputPath, const std::string& outputPath);

};

#endif // COMPRESSOR_H
