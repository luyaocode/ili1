#!/bin/bash
set -euo pipefail  # 严格模式，报错立即退出

# 脚本用法提示
usage() {
    echo "用法：$0 <待加密文件路径>"
    echo "示例：$0 ScreenRecorder-x86_64.AppImage"
    echo "输出：加密后的文件会保存为 <原文件名>.enc，同时生成 SHA256 校验文件 <原文件名>.sha256"
    exit 1
}

# 校验参数：必须传入1个文件路径
if [ $# -ne 1 ]; then
    echo "错误：参数数量不正确！"
    usage
fi

INPUT_FILE="$1"
OUTPUT_FILE="${INPUT_FILE}.enc"
SHA256_FILE="${INPUT_FILE}.sha256"

# 校验输入文件是否存在且可读
if [ ! -f "$INPUT_FILE" ] || [ ! -r "$INPUT_FILE" ]; then
    echo "错误：文件 $INPUT_FILE 不存在或无读取权限！"
    exit 1
fi

# 校验输出文件是否已存在（避免覆盖）
if [ -f "$OUTPUT_FILE" ]; then
    echo "警告：输出文件 $OUTPUT_FILE 已存在！"
    read -p "是否覆盖？(y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "取消加密，退出脚本。"
        exit 0
    fi
fi

# 生成原文件的 SHA256 校验和（用于解密后验证完整性）
echo "正在生成原文件 SHA256 校验和..."
sha256sum "$INPUT_FILE" > "$SHA256_FILE"
echo "✅ 校验文件已保存至 $SHA256_FILE"

# 关键修复：简化 OpenSSL 命令，去掉所有多余换行转义，确保参数正确
echo -e "\n开始加密文件：$INPUT_FILE"
echo "加密算法：AES-256-CBC（带盐值 + PBKDF2 密钥派生）"
echo "请输入加密密码（将隐藏输入）："
openssl enc -aes-256-cbc -salt -pbkdf2 -iter 100000 -in "$INPUT_FILE" -out "$OUTPUT_FILE"

# 加密完成提示
echo -e "\n🎉 加密成功！"
echo "加密文件：$OUTPUT_FILE"
echo "校验文件：$SHA256_FILE"
echo -e "\n提示：解密时需使用对应密码和 decrypt.sh 脚本，解密后建议验证 SHA256 校验和。"
