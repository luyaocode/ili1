#include "Compressor.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <unordered_map>

#include "tool.h"

// === 核心编码常量 (修改点 1) ===
// 使用转义机制来避免标记冲突
const uint8_t ESCAPE_BYTE = 0x1B;                 // 定义转义字节
const uint8_t COMMAND_LZ77_MATCH = 0x00;          // 转义后表示LZ77匹配命令
const uint8_t COMMAND_ESCAPE_LITERAL = 0x1B;      // 转义后表示一个字面量的转义字节
const size_t MIN_MATCH_LENGTH = 3;
const size_t MAX_MATCH_LENGTH = 65535; // 2 bytes
const size_t MAX_MATCH_OFFSET = 65535; // 2 bytes
const size_t WINDOW_SIZE = MAX_MATCH_OFFSET;
const size_t HASH_TABLE_SIZE = 1 << 16; // 65536 buckets

// 将16位无符号整数转换为小端字节序
inline void uint16_to_le_bytes(uint16_t value, char* bytes) {
    bytes[0] = static_cast<char>(value & 0xFF);
    bytes[1] = static_cast<char>((value >> 8) & 0xFF);
}

// 从小端字节序的字节流中解析16位无符号整数
inline uint16_t le_bytes_to_uint16(const char* bytes) {
    return static_cast<uint16_t>(static_cast<uint8_t>(bytes[0]) |
                                 (static_cast<uint16_t>(static_cast<uint8_t>(bytes[1])) << 8));
}

// 快速哈希函数
inline uint32_t fast_hash(uint8_t b1, uint8_t b2, uint8_t b3) {
    return (static_cast<uint32_t>(b1) << 16) | (static_cast<uint32_t>(b2) << 8) | b3;
}

// 在滑动窗口中查找最长匹配
void find_longest_match(const std::vector<char>& data, size_t current_pos,
                        const std::unordered_map<uint32_t, std::vector<size_t>>& hash_table,
                        size_t& best_len, size_t& best_off) {
    best_len = 0;
    best_off = 0;

    if (current_pos + MIN_MATCH_LENGTH > data.size()) {
        return;
    }

    uint32_t current_hash = fast_hash(
        static_cast<uint8_t>(data[current_pos]),
        static_cast<uint8_t>(data[current_pos + 1]),
        static_cast<uint8_t>(data[current_pos + 2])
    );

    auto it = hash_table.find(current_hash);
    if (it != hash_table.end()) {
        const std::vector<size_t>& candidates = it->second;
        // 为了提高效率，可以考虑从最近的位置开始搜索，找到就可以break
        for (auto rit = candidates.rbegin(); rit != candidates.rend(); ++rit) {
            size_t candidate_pos = *rit;
            if (current_pos > candidate_pos && (current_pos - candidate_pos) <= WINDOW_SIZE) {
                size_t len = MIN_MATCH_LENGTH;
                while ((current_pos + len < data.size()) &&
                       (data[candidate_pos + len] == data[current_pos + len]) &&
                       (len < MAX_MATCH_LENGTH)) {
                    len++;
                }
                if (len > best_len) {
                    best_len = len;
                    best_off = current_pos - candidate_pos;
                    if (best_len == MAX_MATCH_LENGTH) {
                        break;
                    }
                }
            }
        }
    }
}

// 辅助函数：安全地将一个字节添加到输出流，如果是转义字节则进行转义 (修改点 2)
inline void write_literal_safely(uint8_t byte, std::vector<char>& output) {
    if (byte == ESCAPE_BYTE) {
        // 如果字节是转义字节，输出转义序列
        output.push_back(static_cast<char>(ESCAPE_BYTE));
        output.push_back(static_cast<char>(COMMAND_ESCAPE_LITERAL));
    } else {
        // 否则，直接输出
        output.push_back(static_cast<char>(byte));
    }
}

// 压缩函数 (修改点 3)
void lz77_compress(const std::vector<char>& data, std::vector<char>& output) {
    if (data.empty()) return;

    size_t current_pos = 0;
    const size_t data_size = data.size();
    std::unordered_map<uint32_t, std::vector<size_t>> hash_table;

    while (current_pos < data_size) {
        size_t best_len = 0;
        size_t best_off = 0;

        // 1. 查找最长匹配
        if (current_pos + MIN_MATCH_LENGTH <= data_size) {
            find_longest_match(data, current_pos, hash_table, best_len, best_off);
        }

        // 2. 编码
        if (best_len >= MIN_MATCH_LENGTH) {
            // 2.1 编码匹配：使用转义序列
            output.push_back(static_cast<char>(ESCAPE_BYTE));
            output.push_back(static_cast<char>(COMMAND_LZ77_MATCH));
            
            char len_bytes[2];
            uint16_to_le_bytes(static_cast<uint16_t>(best_len), len_bytes);
            output.insert(output.end(), len_bytes, len_bytes + 2);
            
            char off_bytes[2];
            uint16_to_le_bytes(static_cast<uint16_t>(best_off), off_bytes);
            output.insert(output.end(), off_bytes, off_bytes + 2);
            
            current_pos += best_len;
        } else {
            // 2.2 编码字面量：安全地写入，防止标记冲突
            write_literal_safely(static_cast<uint8_t>(data[current_pos]), output);
            current_pos++;
        }

        // 3. 更新哈希表 (将当前位置的3字节序列加入)
        // 注意：这里应该在处理完一个字节或一个匹配后，更新当前位置的哈希
        // 原逻辑有些问题，当best_len>1时，会跳过一些位置的哈希更新
        // 一个更稳健的做法是，无论是否匹配，都为当前位置更新哈希
        // 但为了效率，我们只为字面量或匹配的第一个位置更新
        // 这里保持与原逻辑相似，但修复了best_len>1时的问题
        if (current_pos + MIN_MATCH_LENGTH <= data_size) {
             // 只有当我们移动到了一个新的、未被处理的位置时才更新
             // 如果是匹配，current_pos已经跳过了best_len，我们需要为新的current_pos更新
             // 如果是字面量，current_pos只移动了1，我们需要为新的current_pos更新
             // 所以这个位置的逻辑是正确的，它总是为下一个可能的匹配位置更新哈希
            uint32_t h = fast_hash(
                static_cast<uint8_t>(data[current_pos]),
                static_cast<uint8_t>(data[current_pos + 1]),
                static_cast<uint8_t>(data[current_pos + 2])
            );
            hash_table[h].push_back(current_pos);
        }
    }

    // 4. 写入结束标志 (修改点 4)
    // 使用转义序列来表示结束，例如 ESCAPE_BYTE + COMMAND_END
    // 我们可以复用 COMMAND_LZ77_MATCH，但用 len=0 和 off=0 来表示结束
    output.push_back(static_cast<char>(ESCAPE_BYTE));
    output.push_back(static_cast<char>(COMMAND_LZ77_MATCH));
    char end_bytes[4] = {0x00, 0x00, 0x00, 0x00}; // 长度和偏移都为0表示结束
    output.insert(output.end(), end_bytes, end_bytes + 4);
}

// 解压函数 (修改点 5)
void lz77_decompress(const std::vector<char>& data, std::vector<char>& output) {
    if (data.empty()) return;

    size_t pos = 0;
    const size_t data_size = data.size();

    while (pos < data_size) {
        uint8_t current_byte = static_cast<uint8_t>(data[pos]);

        // 1. 检查是否为转义序列的开始
        if (current_byte == ESCAPE_BYTE) {
            if (pos + 1 >= data_size) {
                 throw std::runtime_error("Corrupted data: unexpected end after escape byte.");
            }
            
            uint8_t command_byte = static_cast<uint8_t>(data[pos + 1]);
            pos += 2; // 跳过转义字节和命令字节

            if (command_byte == COMMAND_LZ77_MATCH) {
                // 2. 解码匹配
                if (pos + 4 > data_size) {
                    throw std::runtime_error("Corrupted data: unexpected end while parsing match.");
                }

                uint16_t match_len = le_bytes_to_uint16(&data[pos]);
                pos += 2;
                uint16_t match_off = le_bytes_to_uint16(&data[pos]);
                pos += 2;

                // 如果长度和偏移都为0，表示结束
                if (match_len == 0 && match_off == 0) {
                    break; // 正常结束解压
                }

                if (match_len < MIN_MATCH_LENGTH || match_off == 0 || match_off > output.size()) {
                    throw std::runtime_error("Corrupted data: invalid match parameters.");
                }

                size_t start_idx = output.size() - match_off;
                for (size_t i = 0; i < match_len; ++i) {
                    output.push_back(output[start_idx + i]);
                }
            } 
            else if (command_byte == COMMAND_ESCAPE_LITERAL) {
                // 3. 解码被转义的字面量
                output.push_back(static_cast<char>(ESCAPE_BYTE));
            }
            else {
                // 未知的命令字节，视为数据损坏
                throw std::runtime_error("Corrupted data: unknown command byte after escape.");
            }
        } 
        else {
            // 4. 复制普通字面量
            output.push_back(data[pos]);
            pos++;
        }
    }
}


// 文件操作 (无需修改)
bool Compressor::compress(const std::string& inputPath, const std::string& outputPath) {
    try {
        auto isBinary = isBinaryFile(inputPath);
        std::cout << "Binary file: " << isBinary << std::endl;
        auto result = false;
        if(isBinary)
        {
            result = compressBinary(inputPath,outputPath);
        }
        else
        {
            result = compressText(inputPath,outputPath);
        }
        return result;
    } catch (const std::exception& e) {
        std::cerr << "Compression failed with exception: " << e.what() << std::endl;
        return false;
    }
}

bool Compressor::decompress(const std::string& inputPath, const std::string& outputPath) {
    try {
        std::ifstream ifs(inputPath, std::ios::binary);
        if (!ifs.is_open()) {
            std::cerr << "Error: Could not open input file '" << inputPath << "'." << std::endl;
            return false;
        }
        std::vector<char> compressed_data((std::istreambuf_iterator<char>(ifs)), {});
        ifs.close();

        std::vector<char> decompressed_data;
        lz77_decompress(compressed_data, decompressed_data);

        std::ofstream ofs(outputPath, std::ios::binary);
        if (!ofs.is_open()) {
            std::cerr << "Error: Could not open output file '" << outputPath << "'." << std::endl;
            return false;
        }
        ofs.write(decompressed_data.data(), decompressed_data.size());
        ofs.close();

        std::cout << "Decompression successful." << std::endl;
        std::cout << "Decompressed size: " << decompressed_data.size() << " bytes." << std::endl;

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Decompression failed with exception: " << e.what() << std::endl;
        return false;
    }
}

bool Compressor::compressText(const std::string &inputPath, const std::string &outputPath)
{
    std::ifstream ifs(inputPath, std::ios::binary);
    if (!ifs.is_open()) {
        std::cerr << "Error: Could not open input file '" << inputPath << "'." << std::endl;
        return false;
    }
    std::ofstream ofs(outputPath, std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "Error: Could not open output file '" << outputPath << "'." << std::endl;
        return false;
    }
    std::vector<char> input_data((std::istreambuf_iterator<char>(ifs)), {});
    ifs.close();

    std::vector<char> compressed_data;
    lz77_compress(input_data, compressed_data);

    ofs.write(compressed_data.data(), compressed_data.size());
    ofs.close();
    std::cout << "Compression successful." << std::endl;
    std::cout << "Original size: " << input_data.size() << " bytes." << std::endl;
    std::cout << "Compressed size: " << compressed_data.size() << " bytes." << std::endl;
    double ratio = (1.0 - static_cast<double>(compressed_data.size()) / input_data.size()) * 100.0;
    std::cout << "Compression ratio: " << ratio << "%" << std::endl;
    return true;
}

bool Compressor::compressBinary(const std::string &inputPath, const std::string &outputPath)
{
    return compressText(inputPath,outputPath);
}
