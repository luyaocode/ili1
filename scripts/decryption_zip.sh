#!/bin/bash

if [ $# -ne 1 ];then
	echo "Usage: $0 <dir>"
	exit 1
fi
D=$(realpath "$1") # origin dir
RS=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w 6 | head -n 1)
DC="$(dirname "$D")/$(basename "$D")_${RS}" # copied dir
echo "work: ${DC}"

if [ -d "${DC}" ];then
	rm -rf "${DC}"
fi
cp -r "${D}" "${DC}"
# 支持后缀
for ext in cpp h c hpp pro md; do
	find "${DC}" -type f -name "*.${ext}" | while read -r src; do
	shf=$(echo "${src}" | sed "s/\.${ext}$/.sh/")
	shdir=$(dirname "${shf}")
	[ -d "${shdir}" ] || mkdir -p "${shdir}"
	vim -e -s "${src}" <<EOF
	:w $shf
	:q
EOF
	mv -f "${shf}" "${src}"
	done
done

# 进入 DC 的父目录（即与原始目录同级的目录）
cd "$(dirname "${DC}")" > /dev/null 2>&1

# 以相对路径压缩 DC 目录（此时压缩源是 "report_xY3k9P"，而非完整路径）
zip -r "${DC}.zip" "$(basename "${DC}")" > /dev/null 2>&1

# 回到原来的工作目录（可选，避免影响后续命令）
cd - > /dev/null 2>&1

if [ -d "${DC}" ];then
        rm -rf "${DC}"
fi
echo "done: ${DC}.zip"
