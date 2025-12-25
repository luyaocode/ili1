#include <iostream>
#include <string>
#include "Compressor.h"

std::string remove_lzma_extension(const std::string& str) {
    const std::string suffix = ".lzma";

    // 检查字符串长度是否至少为后缀的长度，并且后缀确实在末尾
    if (str.size() >= suffix.size() &&
        str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0) {
        // 从开头截取，长度为 原长度 - 后缀长度
        return str.substr(0, str.size() - suffix.size());
    }

    // 如果不满足条件，返回原字符串
    return str;
}

void print_usage() {
    std::cout << "Usage:" << std::endl;
    std::cout << "  Compress: " << "ti -c <input_file> [output.lzma]" << std::endl;
    std::cout << "  Decompress: " << "ti -d <input.lzma> [output_file]" << std::endl;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage();
        return 1;
    }
    std::string command = argv[1];
    std::string input = argv[2];
    std::string output;

    bool success = false;
    if (command == "-c") {
        if(argc == 3)
        {
            output = input + ".lzma";
        }
        else
        {
            output = argv[3];
        }
        success = Compressor::compress(input, output);
    } else if (command == "-d") {
        if(argc == 3)
        {
            output = remove_lzma_extension(input);
            if(output == input)
            {
                std::cout << "Error: File suffix is not .lzma." << std::endl;
                return 1;
            }
        }
        else
        {
            output = argv[3];
        }
        success = Compressor::decompress(input, output);
    } else {
        std::cerr << "Error: Unknown command '" << command << "'" << std::endl;
        print_usage();
        return 1;
    }

    return success ? 0 : 1;
}
