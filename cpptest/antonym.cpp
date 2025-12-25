#include <string>
#include <vector>
#include <bitset>
#include <stdexcept>

// 把“01001000”这种 8 位二进制文本转回 uint8_t
static std::uint8_t bin_str_to_byte(const std::string& bits)
{
    if (bits.size() != 8) throw std::invalid_argument("bad bit length");
    return static_cast<std::uint8_t>(std::bitset<8>(bits).to_ulong());
}

// 主功能：按题意求“反义词”
std::string antonym(const std::string& utf8_word)
{
    // 1. 拆成字节
    std::vector<std::uint8_t> bytes(utf8_word.begin(), utf8_word.end());
    std::vector<std::uint8_t> flipped;

    // 2. 对每个字节逐位取反
    for (std::uint8_t b : bytes)
    {
        std::string bits;
        for (int i = 7; i >= 0; --i)          // 高位在前
            bits += ((b >> i) & 1) ? '1' : '0';

        // 3. 取反
        for (char& c : bits) c = (c == '0') ? '1' : '0';

        // 4. 拼回字节
        flipped.push_back(bin_str_to_byte(bits));
    }

    // 5. 重新组装成 UTF-8 字符串
    return std::string(flipped.begin(), flipped.end());
}

/* ---------- 简单测试 ---------- */
#ifdef ANTONYM_DEMO
#include <iostream>
int main()
{
    	while(1)
    	{
    		std::string s;
        	std::cout << "<：";
        	std::getline(std::cin, s);
        	std::cout << ">：" << antonym(s) << '\n';
    	}
}
#endif
