#!/bin/bash

echo "开始监听hardkeyRecordStatus信号... (按Ctrl+C退出)"

# 监听DBus信号并过滤出目标信号行
dbus-monitor --system "type='signal',path='/org/comen/autoTest',interface='org.comen.autoTest',member='hardkeyRecordStatus'" | \
while read -r line; do
    # 只处理包含目标信号成员和boolean的行
    if echo "$line" | grep -q "member=hardkeyRecordStatus"; then
        # 读取下一行（因为boolean值在信号行的下一行）
        read -r value_line
        if echo "$value_line" | grep -q "boolean true"; then
            echo "收到录制状态: 开启 (true)"
        elif echo "$value_line" | grep -q "boolean false"; then
            echo "收到录制状态: 关闭 (false)"
        fi
    fi
done

# 捕获Ctrl+C退出
trap 'echo -e "\n监听已停止"; exit 0' SIGINT
