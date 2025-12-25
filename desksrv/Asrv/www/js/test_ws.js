// 获取DOM元素
const wsIpInput = document.getElementById('wsIp');
const wsPortInput = document.getElementById('wsPort');
const timeoutInput = document.getElementById('timeout');
const checkBtn = document.getElementById('checkBtn');
const autoCheckBtn = document.getElementById('autoCheckBtn');
const stopAutoCheckBtn = document.getElementById('stopAutoCheckBtn');
const statusBox = document.getElementById('statusBox');

let autoCheckInterval = null; // 自动检测定时器

/**
 * 获取当前页面URL的IP/主机名
 * @returns {string} 解析后的IP或默认值127.0.0.1
 */
function getCurrentHostIp() {
    // 处理本地文件协议（file://）
    if (window.location.protocol === 'file:') {
        return '127.0.0.1';
    }

    // 获取主机部分（例：192.168.1.100:8080 或 localhost:3000 或 example.com）
    const host = window.location.host;
    // 分割主机和端口（取第一个部分即为IP/域名）
    const hostName = host.split(':')[0];

    // 可选：将localhost替换为127.0.0.1
    if (hostName === 'localhost') {
        return '127.0.0.1';
    }

    return hostName;
}

/**
 * 检测WebSocket服务器状态
 * @returns {Promise<boolean>}
 */
async function checkWsServerStatus() {
    // 获取输入值
    const ip = wsIpInput.value.trim() || '127.0.0.1';
    const port = wsPortInput.value.trim();
    const timeout = parseInt(timeoutInput.value) || 3000;

    // 验证端口
    if (!port || isNaN(port) || port < 1 || port > 65535) {
        showStatus('请输入有效的端口号(1-65535)', 'error');
        return false;
    }

    // 构建WS连接地址
    const wsUrl = `ws://${ip}:${port}`;

    // 显示加载状态
    showStatus(`正在连接 ${wsUrl} ...`, 'loading');

    return new Promise((resolve) => {
        // 超时处理
        const timeoutTimer = setTimeout(() => {
            showStatus(`连接超时(${timeout}ms)：${wsUrl}`, 'error');
            resolve(false);
        }, timeout);

        // 创建WebSocket连接
        let ws;
        try {
            ws = new WebSocket(wsUrl);
        } catch (err) {
            clearTimeout(timeoutTimer);
            showStatus(`创建连接失败：${err.message}`, 'error');
            resolve(false);
            return;
        }

        // 连接成功
        ws.onopen = () => {
            clearTimeout(timeoutTimer);
            ws.close(1000, '检测完成，主动关闭'); // 正常关闭连接
            showStatus(`✅ 连接成功！WebSocket服务器已启动 (${wsUrl})`, 'success');
            resolve(true);
        };

        // 连接错误
        ws.onerror = (error) => {
            clearTimeout(timeoutTimer);
            showStatus(`❌ 连接失败：${wsUrl} (服务器未启动/地址错误/端口被占用)`, 'error');
            resolve(false);
        };

        // 连接关闭（异常关闭）
        ws.onclose = (event) => {
            if (event.code !== 1000) { // 非正常关闭码
                clearTimeout(timeoutTimer);
                const reason = event.reason || '未知原因';
                showStatus(`❌ 连接关闭：${wsUrl} (${reason})`, 'error');
                resolve(false);
            }
        };
    });
}

/**
 * 显示状态提示
 * @param {string} text 提示文本
 * @param {string} type 类型：success/error/loading
 */
function showStatus(text, type) {
    statusBox.textContent = text;
    statusBox.className = `status ${type}`;
    statusBox.style.display = 'block';
}

// 绑定手动检测按钮事件
checkBtn.addEventListener('click', async () => {
    checkBtn.disabled = true;
    await checkWsServerStatus();
    checkBtn.disabled = false;
});

// 绑定自动检测按钮事件
autoCheckBtn.addEventListener('click', () => {
    if (autoCheckInterval) return;

    // 立即执行一次检测
    checkWsServerStatus();

    // 设置定时器
    autoCheckInterval = setInterval(() => {
        checkWsServerStatus();
    }, 5000);

    // 切换按钮显示状态
    autoCheckBtn.style.display = 'none';
    stopAutoCheckBtn.style.display = 'block';
    showStatus('已开启自动检测，每5秒检测一次', 'loading');
});

// 绑定停止自动检测按钮事件
stopAutoCheckBtn.addEventListener('click', () => {
    clearInterval(autoCheckInterval);
    autoCheckInterval = null;

    // 切换按钮显示状态
    stopAutoCheckBtn.style.display = 'none';
    autoCheckBtn.style.display = 'block';
    showStatus('已停止自动检测', 'loading');
});

// 页面加载完成后执行
window.onload = () => {
    // 设置输入框的默认值为当前URL的IP
    wsIpInput.value = getCurrentHostIp();
    // 聚焦到IP输入框
    wsIpInput.focus();
};
