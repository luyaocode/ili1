#!/bin/bash
set -euo pipefail  # 严格模式，报错立即退出

# 脚本用法提示
usage() {
    echo "用法：$0 <待解密文件路径> [解密后输出文件名]"
    echo "示例1：$0 ScreenRecorder-x86_64.AppImage.enc"
    echo "示例2：$0 ScreenRecorder-x86_64.AppImage.enc ScreenRecorder-dec.AppImage"
    echo "说明："
    echo "  - 若不指定输出文件名，默认输出为去掉 .enc 后缀的原文件名"
    echo "  - 需提前准备原文件的 SHA256 校验文件（<原文件名>.sha256）"
    exit 1
}

# 校验参数数量（1-2个）
if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "错误：参数数量不正确！"
    usage
fi

ENCRYPTED_FILE="$1"
# 处理输出文件名（默认去掉 .enc 后缀）
if [ $# -eq 2 ]; then
    OUTPUT_FILE="$2"
else
    OUTPUT_FILE="${ENCRYPTED_FILE%.enc}"
fi
SHA256_FILE="${ENCRYPTED_FILE%.enc}.sha256"

# 校验加密文件是否存在且可读
if [ ! -f "$ENCRYPTED_FILE" ] || [ ! -r "$ENCRYPTED_FILE" ]; then
    echo "错误：加密文件 $ENCRYPTED_FILE 不存在或无读取权限！"
    exit 1
fi

# 覆盖确认
if [ -f "$OUTPUT_FILE" ]; then
    echo "警告：输出文件 $OUTPUT_FILE 已存在！"
    read -p "是否覆盖？(y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        echo "取消解密，退出脚本。"
        exit 0
    fi
fi

# 核心修复：所有参数写在一行，无换行转义，彻底避免解析错误
echo "开始解密文件：$ENCRYPTED_FILE"
echo "解密算法：AES-256-CBC（带盐值 + PBKDF2 密钥派生）"
echo "请输入加密密码（将隐藏输入）："
openssl enc -d -aes-256-cbc -pbkdf2 -iter 100000 -in "$ENCRYPTED_FILE" -out "$OUTPUT_FILE"

# 完整性验证
if [ -f "$SHA256_FILE" ]; then
    echo -e "\n正在验证文件完整性（SHA256）..."
    if sha256sum -c --status "$SHA256_FILE" --ignore-missing; then
        echo "✅ 完整性验证通过！解密文件与原文件一致。"
    else
        echo "❌ 完整性验证失败！密码错误或文件损坏。"
        rm -f "$OUTPUT_FILE"
        exit 1
    fi
else
    echo -e "\n⚠️  未找到 SHA256 校验文件 $SHA256_FILE，跳过完整性验证。"
fi

# 解密完成提示
echo -e "\n🎉 解密成功！"
echo "解密文件：$OUTPUT_FILE"
if [ ! -x "$OUTPUT_FILE" ]; then
    echo -e "\n提示：若需运行该文件，可执行：chmod +x $OUTPUT_FILE"
fi
