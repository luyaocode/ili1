#!/bin/bash

# 检查参数是否提供
if [ $# -ne 1 ]; then
    echo "用法: $0 <目标目录>"
    exit 1
fi

TARGET_DIR="$1"

# 检查目录是否存在
if [ ! -d "$TARGET_DIR" ]; then
    echo "错误: 目录 '$TARGET_DIR' 不存在"
    exit 1
fi

# 规范化目标目录路径（去除末尾斜杠）
TARGET_DIR=$(realpath "$TARGET_DIR")
TARGET_DIR=${TARGET_DIR%/}

# 临时文件用于存储每个文件的统计结果
FILE_STATS=$(mktemp)

# 查找所有指定类型的文件并统计
find "$TARGET_DIR" -type f \( -name "*.c" -o -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) | while read -r file; do
    # 统计总行数
    total_lines=$(wc -l < "$file")
    
    # 统计有效行数（非空行且非注释行）
    effective_lines=$(sed -e 's/\/\/.*//' -e '/^[[:space:]]*$/d' "$file" | grep -v '^[[:space:]]*/\*' | wc -l)
    
    # 将完整路径转换为相对路径（从目标目录开始）
    relative_path=$(realpath --relative-to="$TARGET_DIR" "$file")
    
    # 保存结果到临时文件（完整路径|相对路径|总行数|有效行数）
    echo "$file|$relative_path|$total_lines|$effective_lines"
done > "$FILE_STATS"

# 检查是否有找到文件
if [ ! -s "$FILE_STATS" ]; then
    echo "在目录 '$TARGET_DIR' 中未找到 .c, .cpp, .h, .hpp 文件"
    rm -f "$FILE_STATS"
    exit 0
fi

# 输出每个文件的统计结果
echo "===== 单个文件的统计结果 ====="
echo -e "总行数\t有效行数\t\t文件路径"  # 数字列在前，文件名在后，两个\t分隔
echo "--------------------------------------------------------"
cat "$FILE_STATS" | while IFS='|' read -r full_path rel_path total effective; do
    echo -e "$total\t$effective\t\t$rel_path"  # 数字列在前，两个\t分隔
done
echo "--------------------------------------------------------"
echo

# 按目录统计并输出
echo "===== 按目录统计结果 ====="
echo -e "文件总数\t总行数\t有效行数\t\t目录路径"  # 数字列在前，目录名在后
echo "--------------------------------------------------------"

# 提取目录路径并统计，数字列在前
awk -F'|' '
{
    rel_path = $2
    total = $3
    effective = $4
    
    # 提取最后一个斜杠的位置
    dir_sep_pos = 0
    for (i = 1; i <= length(rel_path); i++) {
        if (substr(rel_path, i, 1) == "/") {
            dir_sep_pos = i
        }
    }
    
    # 确定相对目录
    if (dir_sep_pos == 0) {
        rel_dir = "."
    } else {
        rel_dir = substr(rel_path, 1, dir_sep_pos - 1)
        if (rel_dir == "") rel_dir = "."
    }
    
    # 累加统计数据
    file_count[rel_dir]++
    total_lines[rel_dir] += total
    effective_lines[rel_dir] += effective
}
END {
    # 输出目录统计（数字列在前，两个\t分隔）
    for (dir in file_count) {
        printf "%d\t%d\t%d\t\t%s\n", file_count[dir], total_lines[dir], effective_lines[dir], dir
    }
}
' "$FILE_STATS" | sort -k4  # 按目录名排序
echo "--------------------------------------------------------"
echo

# 输出根目录的总统计
echo "===== 根目录总统计 ====="
echo "目录路径: ."
awk -F'|' '
BEGIN {
    total_files = 0
    total_total = 0
    total_effective = 0
}
{
    total_files++
    total_total += $3
    total_effective += $4
}
END {
    printf "文件总数: %d\n", total_files
    printf "总行数: %d\n", total_total
    printf "有效行数: %d\n", total_effective
}
' "$FILE_STATS"

# 清理临时文件
rm -f "$FILE_STATS"
