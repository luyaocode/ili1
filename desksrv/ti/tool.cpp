#include "tool.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdint>

// 文件类型枚举
enum class FileType {
    TEXT_ASCII,
    TEXT_UTF8,
    TEXT_UTF16_LE,
    TEXT_UTF16_BE,
    TEXT_UTF32_LE,
    TEXT_UTF32_BE,
    TEXT_OTHER,      // GBK, BIG5等其他文本编码
    BINARY_EXECUTABLE,
    BINARY_IMAGE,
    BINARY_ARCHIVE,
    BINARY_VIDEO,
    BINARY_AUDIO,
    BINARY_OTHER
};

// 获取文件类型的描述字符串
std::string getFileTypeDescription(FileType type) {
    switch (type) {
        case FileType::TEXT_ASCII: return "ASCII Text";
        case FileType::TEXT_UTF8: return "UTF-8 Text";
        case FileType::TEXT_UTF16_LE: return "UTF-16 LE Text";
        case FileType::TEXT_UTF16_BE: return "UTF-16 BE Text";
        case FileType::TEXT_UTF32_LE: return "UTF-32 LE Text";
        case FileType::TEXT_UTF32_BE: return "UTF-32 BE Text";
        case FileType::TEXT_OTHER: return "Other Text Encoding (GBK/BIG5/etc)";
        case FileType::BINARY_EXECUTABLE: return "Binary Executable";
        case FileType::BINARY_IMAGE: return "Image File";
        case FileType::BINARY_ARCHIVE: return "Archive/Compressed File";
        case FileType::BINARY_VIDEO: return "Video File";
        case FileType::BINARY_AUDIO: return "Audio File";
        case FileType::BINARY_OTHER: return "Other Binary File";
        default: return "Unknown";
    }
}

// 魔数检测结构体
struct MagicSignature {
    std::vector<unsigned char> signature;
    size_t offset;
    FileType type;
    std::string description;
};

// 全局魔数数据库
std::vector<MagicSignature> magicSignatures = {
    // 可执行文件
    {{0x7F, 0x45, 0x4C, 0x46}, 0, FileType::BINARY_EXECUTABLE, "ELF Executable"},
    {{0x4D, 0x5A}, 0, FileType::BINARY_EXECUTABLE, "DOS/Windows Executable"},
    {{0xCA, 0xFE, 0xBA, 0xBE}, 0, FileType::BINARY_EXECUTABLE, "Mach-O Executable (BE)"},
    {{0xBE, 0xBA, 0xFE, 0xCA}, 0, FileType::BINARY_EXECUTABLE, "Mach-O Executable (LE)"},

    // 图片文件
    {{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}, 0, FileType::BINARY_IMAGE, "PNG Image"},
    {{0xFF, 0xD8, 0xFF}, 0, FileType::BINARY_IMAGE, "JPEG Image"},
    {{0x47, 0x49, 0x46, 0x38}, 0, FileType::BINARY_IMAGE, "GIF Image"},
    {{0x42, 0x4D}, 0, FileType::BINARY_IMAGE, "BMP Image"},
    {{0x52, 0x49, 0x46, 0x46}, 0, FileType::BINARY_IMAGE, "WebP Image"},

    // 压缩/归档文件
    {{0x50, 0x4B, 0x03, 0x04}, 0, FileType::BINARY_ARCHIVE, "ZIP Archive"},
    {{0x50, 0x4B, 0x05, 0x06}, 0, FileType::BINARY_ARCHIVE, "ZIP Empty Archive"},
    {{0x50, 0x4B, 0x07, 0x08}, 0, FileType::BINARY_ARCHIVE, "ZIP Span"},
    {{0x1F, 0x8B}, 0, FileType::BINARY_ARCHIVE, "GZIP Archive"},
    {{0x28, 0xB5, 0x2F, 0xFD}, 0, FileType::BINARY_ARCHIVE, "ZSTD Archive"},
    {{0x78, 0x01}, 0, FileType::BINARY_ARCHIVE, "LZMA Archive"},
    {{0x52, 0x61, 0x72, 0x21, 0x1A, 0x07}, 0, FileType::BINARY_ARCHIVE, "RAR Archive"},

    // 视频文件
    {{0x00, 0x00, 0x00, 0x18, 0x66, 0x74, 0x79, 0x70}, 0, FileType::BINARY_VIDEO, "MP4 Video"},
    {{0x4F, 0x67, 0x67, 0x53}, 0, FileType::BINARY_VIDEO, "OGG Video/Audio"},
    {{0x1A, 0x45, 0xDF, 0xA3}, 0, FileType::BINARY_VIDEO, "Matroska/WebM Video"},

    // 音频文件
    {{0x49, 0x44, 0x33}, 0, FileType::BINARY_AUDIO, "MP3 Audio"},
    {{0xFF, 0xFB}, 0, FileType::BINARY_AUDIO, "MP3 Audio (Frame Sync)"},
    {{0x57, 0x41, 0x56, 0x45}, 0, FileType::BINARY_AUDIO, "WAV Audio"},

    // PDF文件
    {{0x25, 0x50, 0x44, 0x46}, 0, FileType::BINARY_OTHER, "PDF Document"},
};

// 检测BOM
FileType detectBOM(const std::vector<unsigned char>& header) {
    if (header.size() >= 3 && header[0] == 0xEF && header[1] == 0xBB && header[2] == 0xBF) {
        return FileType::TEXT_UTF8;
    }
    if (header.size() >= 2) {
        if (header[0] == 0xFF && header[1] == 0xFE) {
            return (header.size() >= 4 && header[2] == 0 && header[3] == 0) ?
                FileType::TEXT_UTF32_LE : FileType::TEXT_UTF16_LE;
        }
        if (header[0] == 0xFE && header[1] == 0xFF) {
            return (header.size() >= 4 && header[2] == 0 && header[3] == 0) ?
                FileType::TEXT_UTF32_BE : FileType::TEXT_UTF16_BE;
        }
    }
    if (header.size() >= 4) {
        if (header[0] == 0 && header[1] == 0 && header[2] == 0xFE && header[3] == 0xFF) {
            return FileType::TEXT_UTF32_BE;
        }
    }
    return FileType::BINARY_OTHER; // 无BOM
}

// 魔数检测
FileType detectByMagicNumber(const std::vector<unsigned char>& header) {
    for (const auto& sig : magicSignatures) {
        if (header.size() >= sig.offset + sig.signature.size()) {
            bool match = true;
            for (size_t i = 0; i < sig.signature.size(); ++i) {
                if (header[sig.offset + i] != sig.signature[i]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                return sig.type;
            }
        }
    }
    return FileType::BINARY_OTHER; // 未识别的魔数
}

// 验证UTF-8的合法性
bool isValidUTF8(const std::vector<unsigned char>& data) {
    size_t i = 0;
    while (i < data.size()) {
        if ((data[i] & 0x80) == 0) { // ASCII
            i++;
        } else if ((data[i] & 0xE0) == 0xC0) { // 2字节
            if (i + 1 >= data.size() || (data[i+1] & 0xC0) != 0x80) return false;
            i += 2;
        } else if ((data[i] & 0xF0) == 0xE0) { // 3字节
            if (i + 2 >= data.size() || (data[i+1] & 0xC0) != 0x80 || (data[i+2] & 0xC0) != 0x80) return false;
            i += 3;
        } else if ((data[i] & 0xF8) == 0xF0) { // 4字节
            if (i + 3 >= data.size() || (data[i+1] & 0xC0) != 0x80 || (data[i+2] & 0xC0) != 0x80 || (data[i+3] & 0xC0) != 0x80) return false;
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

// 检测文件类型的主函数
FileType detectFileType(const std::string& filePath) {
    std::ifstream ifs(filePath, std::ios::binary);
    if (!ifs.is_open()) {
        return FileType::BINARY_OTHER;
    }

    // 读取文件头部（前16KB）
    const size_t BUFFER_SIZE = 16384;
    std::vector<unsigned char> buffer(BUFFER_SIZE);
    ifs.read(reinterpret_cast<char*>(buffer.data()), BUFFER_SIZE);
    size_t bytesRead = ifs.gcount();
    ifs.close();

    if (bytesRead == 0) {
        return FileType::TEXT_ASCII; // 空文件视为文本
    }

    // 1. 首先检测BOM
    FileType bomType = detectBOM(buffer);
    if (bomType != FileType::BINARY_OTHER) {
        return bomType;
    }

    // 2. 检测魔数（二进制文件签名）
    FileType magicType = detectByMagicNumber(buffer);
    if (magicType != FileType::BINARY_OTHER) {
        return magicType;
    }

    // 3. 统计分析
    size_t nullBytes = 0, asciiPrintable = 0, asciiControl = 0;
    size_t utf8MultiByte = 0, highByte = 0;

    for (size_t i = 0; i < bytesRead; ) {
        unsigned char c = buffer[i];

        if (c == 0) {
            nullBytes++;
            i++;
        } else if (c < 32) {
            // 常见文本控制符
            if (c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\b') {
                asciiPrintable++;
            } else {
                asciiControl++;
            }
            i++;
        } else if (c <= 126) {
            asciiPrintable++;
            i++;
        } else if (isValidUTF8(std::vector<unsigned char>(buffer.begin() + i, buffer.end()))) {
            // 计算UTF-8多字节字符数
            if ((c & 0xE0) == 0xC0) { i += 2; utf8MultiByte++; }
            else if ((c & 0xF0) == 0xE0) { i += 3; utf8MultiByte++; }
            else if ((c & 0xF8) == 0xF0) { i += 4; utf8MultiByte++; }
            else { i++; highByte++; }
        } else {
            highByte++;
            i++;
        }
    }

    // 计算比例
    double controlRatio = static_cast<double>(asciiControl) / bytesRead;
    double printableRatio = static_cast<double>(asciiPrintable) / bytesRead;

    // 4. 判断UTF-16
    if (nullBytes > bytesRead / 4 && nullBytes < bytesRead / 1.5) {
        // 检查UTF-16 LE模式 (字符\0交替)
        size_t lePatterns = 0;
        for (size_t i = 0; i < bytesRead - 1; i += 2) {
            if ((buffer[i] != 0 && buffer[i+1] == 0 && std::isprint(buffer[i])) ||
                (buffer[i] == 0 && buffer[i+1] != 0 && std::isprint(buffer[i+1]))) {
                lePatterns++;
            }
        }

        if (lePatterns > nullBytes / 2) {
            // 检查是LE还是BE
            size_t leCount = 0, beCount = 0;
            for (size_t i = 0; i < bytesRead - 1; i += 2) {
                if (buffer[i] != 0 && buffer[i+1] == 0 && std::isprint(buffer[i])) leCount++;
                if (buffer[i] == 0 && buffer[i+1] != 0 && std::isprint(buffer[i+1])) beCount++;
            }

            if (leCount > beCount) return FileType::TEXT_UTF16_LE;
            if (beCount > leCount) return FileType::TEXT_UTF16_BE;
        }
    }

    // 5. 判断UTF-8
    if (utf8MultiByte > 0 && isValidUTF8(buffer)) {
        return FileType::TEXT_UTF8;
    }

    // 6. 判断纯ASCII
    if (highByte == 0 && utf8MultiByte == 0) {
        return (controlRatio < 0.05) ? FileType::TEXT_ASCII : FileType::BINARY_OTHER;
    }

    // 7. 判断其他文本编码
    if (printableRatio > 0.7 && controlRatio < 0.1) {
        return FileType::TEXT_OTHER;
    }

    // 8. 其他情况视为二进制
    return FileType::BINARY_OTHER;
}

// 判断是否为二进制文件
bool isBinaryFile(const std::string& filePath) {
    FileType type = detectFileType(filePath);
    std::cout << "FileType: " << getFileTypeDescription(type) << std::endl;
    return !(type == FileType::TEXT_ASCII || type == FileType::TEXT_UTF8 ||
             type == FileType::TEXT_UTF16_LE || type == FileType::TEXT_UTF16_BE ||
             type == FileType::TEXT_UTF32_LE || type == FileType::TEXT_UTF32_BE ||
             type == FileType::TEXT_OTHER);
}
