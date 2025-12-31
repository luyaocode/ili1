let wsconn = null;
let cmdHistory = []; // 指令历史
let historyIndex = -1; // 历史记录索引
let inputBuffer = ''; // 终端输入缓冲区
let term = null; // 终端实例

// 工具函数：字符串转Uint8Array（UTF-8编码）
function stringToUint8Array(str) {
  return new TextEncoder().encode(str);
}

// 工具函数：Uint8Array/Blob转字符串（UTF-8解码）
function uint8ArrayToString(buffer) {
  return new TextDecoder('utf-8').decode(buffer);
}

// 初始化WebSocket连接
function initWs() {
    // WebSocket连接配置（替换为你的后端IP和端口）
    const WS_HOST = "{{WS_HOST}}";
  wsconn = new WebSocket(WS_HOST);

  // 连接成功
  wsconn.onopen = function() {
    appendOutput("[连接成功] 已连接到WebSocket Bash服务器", "info");
    term.prompt(); // 连接成功后显示提示符
  };

// 接收后端二进制消息（核心修改）
  wsconn.onmessage = function(event) {
    // 判断是否为二进制数据（Blob/ArrayBuffer）
    if (event.data instanceof Blob) {
      // 读取Blob类型的二进制数据
      const reader = new FileReader();
      reader.onload = function() {
        // 转为字符串后输出
        const text = uint8ArrayToString(reader.result);
        appendOutput(text, text.startsWith("[错误]") ? "error" : "output");
        term.prompt();
      };
      reader.readAsArrayBuffer(event.data);
    } else if (event.data instanceof ArrayBuffer) {
      // 直接处理ArrayBuffer类型
      const text = uint8ArrayToString(event.data);
      appendOutput(text, text.startsWith("[错误]") ? "error" : "output");
      term.prompt();
    } else {
      // 兼容文本格式（兜底）
      appendOutput(event.data, event.data.startsWith("[错误]") ? "error" : "output");
      term.prompt();
    }
  };

  // 连接关闭
  wsconn.onclose = function() {
    appendOutput("[连接关闭] 与服务器的连接已断开", "error");
    // 尝试重连（可选）
    setTimeout(initWebSocket, 3000);
  };

  // 连接错误
  wsconn.onerror = function(error) {
    appendOutput(`[连接错误] ${error.message}`, "error");
  };
}

// 向终端追加输出（适配Xterm）
function appendOutput(text, className) {
  // 处理换行符，Xterm需要\r\n
  const formattedText = text.replace(/\n/g, '\r\n');
  // 根据类型设置不同颜色
  switch(className) {
    case "info":
      term.write(`\x1B[32m${formattedText}\x1B[0m`); // 绿色
      break;
    case "error":
      term.write(`\x1B[31m${formattedText}\x1B[0m`); // 红色
      break;
    default:
      term.write(formattedText); // 默认白色
      break;
  }
}

// 发送指令到后端（二进制格式）
function sendCommand(cmd) {
  if (!cmd || !wsconn || wsconn.readyState !== WebSocket.OPEN) return;

  // 记录指令历史
  cmdHistory.push(cmd);
  historyIndex = cmdHistory.length;

  // 核心修改：将字符串转为二进制（UTF-8）后发送
  const binaryData = stringToUint8Array(cmd);
  wsconn.send(binaryData);

  // 清空输入缓冲区
  inputBuffer = '';
}

// 初始化终端
function initTerminal() {
  // 创建终端实例
  term = new Terminal({
    cols: 80,
    rows: 24,
    fontSize: 14,
    theme: {
      background: '#1E1E1E',
      foreground: '#E0E0E0'
    }
  });

  // 挂载到DOM元素
  term.open(document.getElementById('terminal'));

  // 自定义prompt方法
  term.prompt = function() {
    this.write('\r\n$ '); // Bash风格提示符
  };

  // 处理终端输入
  term.onData((data) => {
    switch (data) {
      case '\r': // 回车：执行命令
        if (inputBuffer.trim()) {
          // 显示用户输入的命令
          term.write('\r\n');
          sendCommand(inputBuffer.trim());
        } else {
          term.prompt(); // 空命令直接显示新提示符
        }
        break;
      case '\x7F': // 退格键 (Backspace)
        if (inputBuffer.length > 0) {
          inputBuffer = inputBuffer.slice(0, -1);
          term.write('\b \b');
        }
        break;
      case '\x1B[A': // 上箭头：上一条历史
        if (historyIndex > 0) {
          historyIndex--;
          // 清空当前输入并显示历史命令
          term.write('\r\x1B[K$ '); // 清除当前行并显示提示符
          inputBuffer = cmdHistory[historyIndex];
          term.write(inputBuffer);
        }
        break;
      case '\x1B[B': // 下箭头：下一条历史
        if (historyIndex < cmdHistory.length - 1) {
          historyIndex++;
          term.write('\r\x1B[K$ ');
          inputBuffer = cmdHistory[historyIndex];
          term.write(inputBuffer);
        } else {
          historyIndex = cmdHistory.length;
          term.write('\r\x1B[K$ ');
          inputBuffer = '';
        }
        break;
      case '\x03': // Ctrl+C
        inputBuffer = '';
        term.write('\r\n^C');
        term.prompt();
        break;
      default: // 普通字符输入
        if (data >= String.fromCharCode(0x20) && data <= String.fromCharCode(0x7E)) {
          inputBuffer += data;
          term.write(data);
        }
        break;
    }
  });

  // 初始欢迎信息
  term.write("请等待连接服务器...\r\n");
}

// 页面加载完成后初始化
document.addEventListener('DOMContentLoaded', () => {
  initTerminal(); // 先初始化终端
  initWs(); // 再初始化WebSocket连接
});
