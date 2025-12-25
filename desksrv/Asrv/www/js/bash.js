// WebSocket连接配置（替换为你的后端IP和端口）
const WS_HOST = "{{WS_HOST}}";
let ws = null;
let cmdHistory = []; // 指令历史
let historyIndex = -1; // 历史记录索引

// 初始化WebSocket连接（关键修改：声明接收二进制类型）
function initWebSocket() {
    ws = new WebSocket(WS_HOST);
    // 核心配置：指定WebSocket接收二进制数据的格式为ArrayBuffer（适配QByteArray）
    ws.binaryType = "arraybuffer";

    // 连接成功
    ws.onopen = function() {
        appendOutput("[连接成功] 已连接到WebSocket Bash服务器", "info");
    };

    // 接收后端消息（关键修改：解析QByteArray传递的二进制数据）
    ws.onmessage = function(event) {
        let receivedData = "";
        // 判断是否为二进制数据（QByteArray传递的是ArrayBuffer类型）
        if (event.data instanceof ArrayBuffer) {
            // 将ArrayBuffer转换为Uint8Array，再解码为UTF-8字符串
            const uint8Array = new Uint8Array(event.data);
            receivedData = new TextDecoder("utf-8").decode(uint8Array);
        } else {
            // 兼容普通文本格式（可选）
            receivedData = event.data;
        }
        // 正常判断输出类型
        appendOutput(receivedData, receivedData.startsWith("[错误]") ? "error" : "output");
    };

    // 连接关闭
    ws.onclose = function() {
        appendOutput("[连接关闭] 与服务器的连接已断开", "error");
        document.getElementById("cmd-input").disabled = true;
        // 尝试重连（可选）
        setTimeout(initWebSocket, 3000);
    };

    // 连接错误
    ws.onerror = function(error) {
        appendOutput(`[连接错误] ${error.message}`, "error");
    };
}

// 向终端追加输出（无修改）
function appendOutput(text, className) {
    const terminal = document.getElementById("terminal");
    const div = document.createElement("div");
    div.className = className;
    // 处理换行符
    div.textContent = text;
    terminal.appendChild(div);
    // 自动滚动到底部
    terminal.scrollTop = terminal.scrollHeight;
}

// 发送指令到后端（关键修改：将指令转换为QByteArray兼容的二进制格式）
function sendCommand() {
    const input = document.getElementById("cmd-input");
    const cmd = input.value.trim();
    if (!cmd || !ws || ws.readyState !== WebSocket.OPEN) return;

    // 记录指令历史
    cmdHistory.push(cmd);
    historyIndex = cmdHistory.length;

    // 核心修改：将字符串指令转换为Uint8Array（QByteArray可直接接收的二进制格式）
    const uint8Array = new TextEncoder("utf-8").encode(cmd);
    // 发送二进制数据（替代原始的字符串发送）
    ws.send(uint8Array);

    // 清空输入框
    input.value = "";

    // 显示输入的指令（模拟Bash提示符）
    appendOutput(`$ ${cmd}`, "output");
}

// 绑定键盘事件（无修改）
document.getElementById("cmd-input").addEventListener("keydown", function(e) {
    // 回车执行指令
    if (e.key === "Enter") {
        sendCommand();
        e.preventDefault();
    }
    // 上箭头：上一条历史
    else if (e.key === "ArrowUp") {
        if (historyIndex > 0) {
            historyIndex--;
            this.value = cmdHistory[historyIndex];
        }
        e.preventDefault();
    }
    // 下箭头：下一条历史
    else if (e.key === "ArrowDown") {
        if (historyIndex < cmdHistory.length - 1) {
            historyIndex++;
            this.value = cmdHistory[historyIndex];
        } else {
            historyIndex = cmdHistory.length;
            this.value = "";
        }
        e.preventDefault();
    }
});

// 页面加载时初始化
window.onload = initWebSocket;
