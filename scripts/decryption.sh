#!/bin/bash

# 目录模式下需要处理的文件后缀（空格分隔）
SUPPORTED_EXTS="cpp h c hpp pro md"

# 检查参数数量
if [ $# -ne 1 ]; then
    echo "Usage: $0 <dir|file>"
    exit 1
fi

# 获取输入路径的真实路径
INPUT_PATH=$(realpath "$1")

# 检查输入路径是否存在
if [ ! -e "${INPUT_PATH}" ]; then
    echo "错误: 路径 '${INPUT_PATH}' 不存在！"
    exit 1
fi

# 生成随机字符串（6位字母数字）
RS=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 6 | head -n 1)

# 定义备份路径（原路径+随机字符）
BACKUP_PATH="${INPUT_PATH}_${RS}"

# 核心处理逻辑
if [ -d "${INPUT_PATH}" ]; then
    # ===================== 目录模式 =====================
    echo "=== 处理目录: ${INPUT_PATH} ==="
    
    # 1. 先备份原目录（重命名）
    echo "1. 备份原目录到: ${BACKUP_PATH}"
    if [ -d "${BACKUP_PATH}" ]; then
        echo "警告: 备份目录已存在，先删除旧备份..."
        rm -rf "${BACKUP_PATH}"
    fi
    mv "${INPUT_PATH}" "${BACKUP_PATH}"
    
    # 2. 复制备份目录到原路径（作为待处理的新目录）
    echo "2. 复制备份目录到原路径..."
    cp -r "${BACKUP_PATH}" "${INPUT_PATH}"
    
    # 3. 处理原路径下指定后缀的文件
    echo "3. 开始处理目录内文件..."
    for ext in ${SUPPORTED_EXTS}; do
        echo "   正在处理 .${ext} 后缀文件..."
        find "${INPUT_PATH}" -type f -name "*.${ext}" | while read -r src; do
            # 生成临时sh文件路径
            shf=$(echo "${src}" | sed "s/\.${ext}$/.sh/")
            shdir=$(dirname "${shf}")
            
            # 确保目录存在
            [ -d "${shdir}" ] || mkdir -p "${shdir}"
            
            # 使用vim转换文件
            vim -e -s "${src}" <<EOF
:w $shf
:q
EOF
            
            # 替换原文件（处理后的文件覆盖原文件）
            mv -f "${shf}" "${src}"
        done
    done

elif [ -f "${INPUT_PATH}" ]; then
    # ===================== 文件模式 =====================
    echo "=== 处理文件: ${INPUT_PATH} ==="
    
    # 1. 先备份原文件（重命名）
    echo "1. 备份原文件到: ${BACKUP_PATH}"
    if [ -f "${BACKUP_PATH}" ]; then
        echo "警告: 备份文件已存在，先删除旧备份..."
        rm -f "${BACKUP_PATH}"
    fi
    mv "${INPUT_PATH}" "${BACKUP_PATH}"
    
    # 2. 复制备份文件到原路径（作为待处理的新文件）
    echo "2. 复制备份文件到原路径..."
    cp "${BACKUP_PATH}" "${INPUT_PATH}"
    
    # 3. 处理单个文件
    echo "3. 开始处理文件..."
    shf="${INPUT_PATH}.sh"
    shdir=$(dirname "${shf}")
    [ -d "${shdir}" ] || mkdir -p "${shdir}"
    
    # 使用vim转换文件
    vim -e -s "${INPUT_PATH}" <<EOF
:w $shf
:q
EOF
    
    # 替换原文件（处理后的文件覆盖原文件）
    mv -f "${shf}" "${INPUT_PATH}"

fi

echo -e "\n=== 操作完成 ==="
echo "处理后的文件/目录: ${INPUT_PATH}"
echo "备份文件/目录: ${BACKUP_PATH}"
