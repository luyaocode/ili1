#!/bin/bash
# 可执行文件合并脚本（自动按序合并所有拆分文件，删除片段）
# 用法：直接运行 ./merge_exec.sh 即可

# 查找当前目录下所有拆分文件（格式：xxx_part【数字】.bin）
part_files=$(ls -1 ./*_part*.bin 2>/dev/null | grep -E "_part[0-9]+\.bin$")

# 检查是否找到拆分文件
if [ -z "$part_files" ]; then
    echo "错误：当前目录未找到符合格式的拆分文件（格式：xxx_part【数字】.bin）！"
    exit 1
fi

# 提取拆分文件的基础名称（如 "ScreenRecorder-x86_64.AppImage" 从 "ScreenRecorder-x86_64.AppImage_part1.bin" 中提取）
# 取第一个拆分文件解析基础名称
first_part_file=$(echo "$part_files" | head -n1)
BASE_NAME=$(echo "$first_part_file" | sed -E 's/_part[0-9]+\.bin$//')

# 验证所有拆分文件是否属于同一基础名称（避免混合不同文件的片段）
valid_part_files=$(ls -1 ./${BASE_NAME}_part*.bin 2>/dev/null | grep -E "_part[0-9]+\.bin$")
if [ -z "$valid_part_files" ]; then
    echo "错误：未找到属于同一文件的拆分片段！"
    exit 1
fi

# 按序号排序拆分文件（关键：使用版本号排序，确保 part1 → part2 → ... → partN 顺序）
sorted_part_files=$(echo "$valid_part_files" | sort -V)

# 检查排序后的文件是否连续（如 part1、part2、part3 无缺失）
max_index=0
for file in $sorted_part_files; do
    # 提取文件中的序号（如从 "test_exec_part3.bin" 提取 3）
    index=$(echo "$file" | sed -E 's/.*_part([0-9]+)\.bin/\1/')
    if [ $index -ne $((max_index + 1)) ]; then
        echo "警告：拆分文件序号不连续（缺失第 $((max_index + 1)) 个片段），仍尝试合并现有片段..."
    fi
    max_index=$index
done

# 合并后的目标文件名为基础名称（去掉_part*部分）
MERGED_FILE="$BASE_NAME"

# 执行合并（按排序后的顺序拼接所有片段）
echo "正在合并拆分文件（共 $max_index 个片段）→ $MERGED_FILE..."
cat $sorted_part_files > "$MERGED_FILE"

# 检查合并是否成功
if [ ! -f "$MERGED_FILE" ] || [ ! -s "$MERGED_FILE" ]; then
    echo "错误：合并失败！"
    rm -f "$MERGED_FILE"
    exit 1
fi

# 恢复可执行权限（拆分时权限丢失，必须手动恢复）
chmod +x "$MERGED_FILE"

# 计算合并后文件大小
merged_size=$(stat -c "%s" "$MERGED_FILE")
echo -e "\n合并成功！"
echo "合并后文件：$MERGED_FILE（大小：$merged_size 字节）"
echo "已自动添加可执行权限"

# 删除所有拆分片段（核心需求：合并后删除片段）
rm -f $sorted_part_files
echo "已删除所有拆分文件（共 $max_index 个）："
echo "$sorted_part_files"

exit 0
