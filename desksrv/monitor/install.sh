#!/bin/bash

# 检查是否以root权限运行（部分操作需要root）
if [ "$(id -u)" -ne 0 ]; then
    echo "请使用sudo或root权限运行此脚本" >&2
    exit 1
fi

# 进入脚本目录
cd "$(dirname "$0")"
APP_PATH=$(pwd)
echo "APP_PATH: ${APP_PATH}"
CURRENT_USER="${SUDO_USER:-$(logname 2>/dev/null || echo $USER)}"
echo "USER: ${CURRENT_USER}"

# 配置用户组
sudo groupadd -f ${SUDO_USER}
sudo usermod -aG ${SUDO_USER} ${SUDO_USER}

mkdir -p /data/logs/monitor

# 设置目录权限
chown ${SUDO_USER}:${SUDO_USER} /data/logs/monitor  # 所有者改为服务用户/组
chmod 755 /data/logs/monitor                  # 设置读写执行权限

# 检查 monitor 文件是否存在
if [ ! -f "./monitor" ]; then
    echo "错误：monitor 文件不存在！"
    exit 1
fi
# 检查 monitor 是否带有调试信息
if file ./monitor | grep -qiE "debug|not stripped"; then
    echo "错误：monitor 程序为debug版本，不能部署到 /usr/bin！"
    exit 1
fi
# 复制程序文件到/usr/bin
echo "正在复制程序文件..."
sudo cp -f monitor /usr/bin || { echo "复制monitor失败"; exit 1; }
sudo cp -f reminder /usr/bin || { echo "复制reminder失败"; exit 1; }
sudo cp -f ScreenRecorder /usr/bin || { echo "复制ScreenRecorder失败"; exit 1; }
sudo cp -f PasteFlow /usr/bin || { echo "复制PasteFlow失败"; exit 1; }
sudo cp -f ti /usr/bin || { echo "复制ti失败"; exit 1; }
sudo cp -f StegoTool /usr/bin || { echo "复制StegoTool失败"; exit 1; }
sudo cp -f QuickManager /usr/bin || { echo "复制QuickManager失败"; exit 1; }
sudo cp -f ScheduleTaskMgr /usr/bin || { echo "复制ScheduleTaskMgr失败"; exit 1; }
sudo cp -f Asrv/Asrv /usr/bin || { echo "复制Asrv失败"; exit 1; }
unlink /usr/bin/www
sudo ln -s ${APP_PATH}/Asrv/www /usr/bin/www || { echo "创建软链接/usr/bin/www失败"; exit 1; }

# 复制动态库到/usr/lib
echo "正在复制动态库..."
sudo cp -d -f libcommontool.so* /usr/lib || { echo "复制mlibcommontool失败"; exit 1; }
sudo ldconfig

# 复制服务配置文件
echo "正在复制服务配置..."
sed -e "s|%USER%|${CURRENT_USER}|g" -e "s|%APP_PATH%|${APP_PATH}|g" monitor.service > /etc/systemd/system/monitor.service || {
  echo "错误：替换并生成monitor.service失败"
  exit 1
}

sudo cp -f monitor.conf /etc || { echo "复制monitor.conf失败"; exit 1; }

# 确保日志目录存在（如果服务配置了日志文件）
# 如果你使用了之前的日志配置，添加这一步确保目录存在
echo "检查日志目录..."
sudo mkdir -p /data/logs/monitor || { echo "创建日志目录失败"; exit 1; }
sudo chmod 755 /data/logs/monitor  # 赋予基本权限

# 重载monitor配置
echo "重载monitor配置..."
sudo systemctl daemon-reload || { echo "重载服务配置失败"; exit 1; }
sudo systemctl enable monitor || { echo "启用服务失败"; exit 1; }
echo "重启monitor服务..."
sudo systemctl stop monitor || true
sudo systemctl start monitor || { echo "启动monitor失败"; exit 1; }
# 检查服务状态
echo "服务状态："
sudo systemctl status monitor --no-pager
###############################################################################################################################
# 【启动其他服务】
echo "停止 PasteFlow"
# 查找并杀死 PasteFlow 进程（排除grep自身）
ps -ef | grep -v grep | grep /usr/bin/PasteFlow | awk '{print $2}' | xargs -r kill -9 >/dev/null 2>&1
CURRENT_UID=$(id -u "${CURRENT_USER}")
CURRENT_HOME=$(eval echo ~${CURRENT_USER})  # 获取普通用户的实际家目录
touch /tmp/PasteFlow.log
chown ${CURRENT_USER}:${CURRENT_USER} /tmp/PasteFlow.log
echo "启动 PasteFlow"
sudo -u "${CURRENT_USER}" \
    env DISPLAY="$DISPLAY" \
    XDG_RUNTIME_DIR="/run/user/${CURRENT_UID}" \
    XAUTHORITY="${CURRENT_HOME}/.Xauthority" \
    HOME="${CURRENT_HOME}" \
    USER="${CURRENT_USER}" \
    bash -c "nohup /usr/bin/PasteFlow > /tmp/PasteFlow.log 2>&1 &"

echo "停止 QuickManager"
# 查找并杀死 QuickManager 进程（排除grep自身）
ps -ef | grep -v grep | grep /usr/bin/QuickManager | awk '{print $2}' | xargs -r kill -9 >/dev/null 2>&1
touch /tmp/QuickManager.log
chown ${CURRENT_USER}:${CURRENT_USER} /tmp/QuickManager.log
echo "启动 QuickManager"
sudo -u "${CURRENT_USER}" \
    env DISPLAY="$DISPLAY" \
    XDG_RUNTIME_DIR="/run/user/${CURRENT_UID}" \
    XAUTHORITY="${CURRENT_HOME}/.Xauthority" \
    HOME="${CURRENT_HOME}" \
    USER="${CURRENT_USER}" \
    bash -c "nohup /usr/bin/QuickManager > /tmp/QuickManager.log 2>&1 &"

echo "停止 ScheduleTaskMgr"
# 查找并杀死 ScheduleTaskMgr 进程（排除grep自身）
ps -ef | grep -v grep | grep /usr/bin/ScheduleTaskMgr | awk '{print $2}' | xargs -r kill -9 >/dev/null 2>&1
touch /tmp/ScheduleTaskMgr.log
chown ${CURRENT_USER}:${CURRENT_USER} /tmp/ScheduleTaskMgr.log
echo "启动 ScheduleTaskMgr"
sudo -u "${CURRENT_USER}" \
    env DISPLAY="$DISPLAY" \
    XDG_RUNTIME_DIR="/run/user/${CURRENT_UID}" \
    XAUTHORITY="${CURRENT_HOME}/.Xauthority" \
    HOME="${CURRENT_HOME}" \
    USER="${CURRENT_USER}" \
    bash -c "nohup /usr/bin/ScheduleTaskMgr > /tmp/ScheduleTaskMgr.log 2>&1 &"

echo "停止 Asrv"
ps -ef | grep -v grep | grep /usr/bin/Asrv | awk '{print $2}' | xargs -r kill -9 >/dev/null 2>&1
touch /tmp/Asrv.log
chown ${CURRENT_USER}:${CURRENT_USER} /tmp/Asrv.log
echo "启动 Asrv"
sudo -u "${CURRENT_USER}" \
    env DISPLAY="$DISPLAY" \
    XDG_RUNTIME_DIR="/run/user/${CURRENT_UID}" \
    XAUTHORITY="${CURRENT_HOME}/.Xauthority" \
    HOME="${CURRENT_HOME}" \
    USER="${CURRENT_USER}" \
    bash -c "nohup /usr/bin/Asrv >> /tmp/Asrv.log 2>&1 &"

###############################################################################################################################
echo "操作完成"
