// 1. 初始化 WebSocket
const WS = new WebSocket("{{WS_HOST}}");
WS.binaryType = 'arraybuffer';

// 2. 初始化终端（完全前端控制显示）
const term = new Terminal({
   cols: 80, rows: 24,
   screenKeys: true,  // 前端接管快捷键
   cursorBlink: true,
   useStyle: true,
   convertEol: true,
   echo: false        // 关闭终端默认回显（关键：避免终端自动回显导致重复发送）
});
term.open(document.getElementById('terminal'));

// 核心变量
let inputBuffer = '';  // 输入缓存：存储用户输入的完整命令
let prompt = '$ ';     // 模拟终端提示符（和后端保持一致）
let isCommandExecuting = false; // 避免重复发送

// 初始化：显示提示符
term.write(prompt);

// 3. 完全接管前端输入：仅前端显示，仅回车时发送完整命令到后端
term.on('data', data => {
   if (isCommandExecuting) return; // 命令执行中忽略所有输入

   // 处理不同输入类型
   switch (data) {
       // 回车/换行：唯一的发送时机
       case '\r':
       case '\n':
           // 处理空回车：仅换行+重新显示提示符
           if (inputBuffer.trim() === '') {
               term.write('\n' + prompt);
               return;
           }

           // 确保WebSocket已连接
           if (WS.readyState === WebSocket.OPEN) {
               isCommandExecuting = true; // 标记为执行中，禁止新输入
               // 发送完整命令（拼接回车，匹配终端命令执行习惯）
               WS.send(new TextEncoder().encode(inputBuffer + '\n'));
               // 前端换行（和后端返回结果分隔）
               term.write('\n');
               // 清空输入缓存
               inputBuffer = '';
           } else {
               // 提示WebSocket未连接
               term.write('\n[错误] WebSocket未连接，请重试\n' + prompt);
               inputBuffer = '';
           }
           break;

       // 退格键（两种常见的退格编码）：仅前端处理，不发送到后端
       case '\b': // 标准退格
       case '\x7f': // DEL键（部分键盘/终端的退格编码）
           if (inputBuffer.length > 0) {
               // 从缓存中删除最后一个字符
               inputBuffer = inputBuffer.slice(0, -1);
               // 前端模拟退格效果：光标回退→空格覆盖→光标再回退
               term.write('\b \b');
           }
           break;

       // 普通字符（字母、数字、符号等）：仅存入缓存+前端显示，不发送
       default:
           // 过滤不可见字符（可选：避免缓存无效字符）
           if (data.length === 1 && data.charCodeAt(0) >= 32) {
               inputBuffer += data;
               term.write(data); // 前端实时显示输入的字符
           }
           break;
   }
});

// 4. 后端执行结果处理：渲染结果+恢复输入
WS.onmessage = e => {
   let output = '';
   // 处理不同类型的后端返回数据
   if (e.data instanceof ArrayBuffer) {
       output = new TextDecoder().decode(e.data);
   } else if (typeof e.data === 'string') {
       output = e.data;
   } else {
       output = String(e.data);
   }

   // 渲染后端返回的执行结果
   term.write(output);
   // 执行完成：显示新提示符，恢复输入能力
   term.write('\n' + prompt);
   isCommandExecuting = false;
};

// 补充：WebSocket连接异常处理（可选，提升鲁棒性）
WS.onerror = (error) => {
   term.write(`\n[WebSocket错误] ${error.message}\n${prompt}`);
   isCommandExecuting = false;
};

WS.onclose = () => {
   term.write(`\n[WebSocket断开] 连接已关闭\n${prompt}`);
   isCommandExecuting = false;
};
