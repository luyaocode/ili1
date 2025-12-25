#!/bin/bash
# 可执行文件分割脚本（支持任意份数，确保每个片段至少1字节，不保留原文件）
# 用法：./split_exec.sh 目标可执行文件路径 拆分份数

# 检查参数是否完整
if [ $# -ne 2 ]; then
    echo "错误：参数不足或错误！"
    echo "用法：$0 目标可执行文件路径 拆分份数（正整数，≥1）"
    exit 1
fi

# 定义变量
TARGET_FILE="$1"
SPLIT_COUNT="$2"
BASE_NAME=$(basename "$TARGET_FILE")  # 提取原文件名称（不含路径）
PART_PREFIX="${BASE_NAME}_part"      # 拆分文件前缀（如 "test_exec_part"）

# 检查拆分份数是否为有效正整数（≥1）
if ! [[ "$SPLIT_COUNT" =~ ^[1-9][0-9]*$ ]]; then
    echo "错误：拆分份数必须是 ≥1 的正整数！"
    exit 1
fi

# 检查目标文件是否存在
if [ ! -f "$TARGET_FILE" ]; then
    echo "错误：文件 $TARGET_FILE 不存在！"
    exit 1
fi

# 检查文件是否可执行（仅警告，不阻止拆分）
if [ ! -x "$TARGET_FILE" ]; then
    echo "警告：文件 $TARGET_FILE 不是可执行文件，仍将尝试拆分..."
fi

# 获取文件大小（字节）
FILE_SIZE=$(stat -c "%s" "$TARGET_FILE")
echo "原文件大小：$FILE_SIZE 字节，计划拆分份数：$SPLIT_COUNT"

# 处理拆分份数超过文件大小的情况（确保每个片段至少1字节）
if [ "$SPLIT_COUNT" -gt "$FILE_SIZE" ]; then
    echo "警告：拆分份数（$SPLIT_COUNT）超过文件大小（$FILE_SIZE 字节）"
    echo "自动调整为：前 $FILE_SIZE 个片段各1字节，剩余 $((SPLIT_COUNT - FILE_SIZE)) 个片段为1字节空文件"
fi

# 清理可能存在的旧拆分文件（避免冲突）
rm -f "${PART_PREFIX}"*.bin

# 执行拆分（循环生成每个片段）
echo -e "\n正在拆分文件：$TARGET_FILE → $SPLIT_COUNT 个片段..."
current_offset=0  # 当前读取偏移量（字节）

for ((i=1; i<=SPLIT_COUNT; i++)); do
    part_file="${PART_PREFIX}${i}.bin"  # 拆分文件名（如 "test_exec_part1.bin"）
    
    # 计算当前片段的大小：
    # 1. 当拆分份数 ≤ 文件大小时，按比例分配（前REMAIN_BYTES个多1字节）
    # 2. 当拆分份数 > 文件大小时，前FILE_SIZE个为1字节，剩余为1字节空文件
    if [ "$SPLIT_COUNT" -le "$FILE_SIZE" ]; then
        BASE_SPLIT_SIZE=$((FILE_SIZE / SPLIT_COUNT))
        REMAIN_BYTES=$((FILE_SIZE % SPLIT_COUNT))
        current_size=$((i <= REMAIN_BYTES ? BASE_SPLIT_SIZE + 1 : BASE_SPLIT_SIZE))
    else
        if [ "$i" -le "$FILE_SIZE" ]; then
            current_size=1  # 前FILE_SIZE个片段各1字节（取自原文件）
        else
            current_size=1  # 剩余片段为1字节空文件
            # 提前设置偏移量为文件末尾（避免读取有效数据）
            current_offset=$FILE_SIZE
        fi
    fi
    
    # 使用dd拆分（按偏移量读取，避免重复）
    # 当偏移量超过文件大小时，生成空文件（但保证1字节）
    dd if="$TARGET_FILE" of="$part_file" bs=1 count="$current_size" skip="$current_offset" 2>/dev/null
    
    # 特殊处理：当需要空文件但dd生成0字节时，强制写入1字节（满足最小文件要求）
    if [ "$current_size" -eq 1 ] && [ ! -s "$part_file" ]; then
        echo -n "" | dd of="$part_file" bs=1 count=1 2>/dev/null
    fi
    
    # 检查当前片段是否生成成功（必须至少1字节）
    part_size=$(stat -c "%s" "$part_file" 2>/dev/null)
    if [ "$part_size" -ne 1 ] && [ "$current_size" -eq 1 ]; then
        echo "错误：拆分第 $i 个片段失败（无法生成1字节文件）！"
        rm -f "${PART_PREFIX}"*.bin  # 清理已生成的片段
        exit 1
    elif [ "$current_size" -gt 1 ] && [ "$part_size" -ne "$current_size" ]; then
        echo "错误：拆分第 $i 个片段失败（大小不匹配）！"
        rm -f "${PART_PREFIX}"*.bin  # 清理已生成的片段
        exit 1
    fi
    
    echo "生成片段：$part_file（大小：$part_size 字节）"
    # 仅在还有有效数据时更新偏移量
    if [ "$current_offset" -lt "$FILE_SIZE" ]; then
        current_offset=$((current_offset + current_size))
    fi
done

# 计算拆分后总大小
total_part_size=0
for part_file in "${PART_PREFIX}"*.bin; do
    total_part_size=$((total_part_size + $(stat -c "%s" "$part_file")))
done

echo -e "\n拆分完成！"
echo "拆分后总大小：$total_part_size 字节（原文件大小：$FILE_SIZE 字节）"

# 删除原文件（核心需求：不保留原文件）
rm -f "$TARGET_FILE"
echo "原文件已删除：$TARGET_FILE"
echo "拆分文件列表："
ls -1 "${PART_PREFIX}"*.bin | sort -V  # 按版本号排序显示（确保顺序正确）

exit 0
