// 配置项
const WS_HOST = "{{WS_HOST}}";

// 全局变量
let ws = null;
const canvas = document.getElementById('screenCanvas');
const ctx = canvas.getContext('2d');
const statusEl = document.getElementById('status');
let screenWidth = 0;
let screenHeight = 0;

// 键盘状态跟踪：记录组合键是否按下
const keyState = {
    ctrl: false,
    shift: false,
    alt: false,
    meta: false, // Windows键/Command键
    pressedKeys: new Set() // 记录当前按下的普通按键
};

// 初始化WebSocket连接
function initWebSocket() {
    // 关闭原有连接
    if (ws) {
        ws.close();
    }

    // 创建新连接
    ws = new WebSocket(WS_HOST);

    // 存储当前帧的元信息（宽高）
    let currentFrameMeta = null;

    // 连接成功
    ws.onopen = () => {
        console.log('WebSocket connected');
        statusEl.className = 'status online';
        statusEl.textContent = '已连接';
    };

    // 接收消息
    ws.onmessage = (event) => {
        // 判断是否是二进制数据
        if (event.data instanceof Blob) {
            // 处理JPEG二进制帧
            if (currentFrameMeta) {
                renderJpegFrame(event.data, currentFrameMeta);
                currentFrameMeta = null; // 重置元信息
            }
        } else {
            // 处理文本JSON消息
            const data = JSON.parse(event.data);
            if (data.type === 'screen_frame_meta') {
                // 保存帧元信息，等待二进制数据
                currentFrameMeta = {
                    width: data.width,
                    height: data.height
                };
            }
            // 保留原有键鼠事件处理（如果后端有响应的话）
        }
    };

    // 连接关闭
    ws.onclose = () => {
        console.log('WebSocket disconnected');
        statusEl.className = 'status offline';
        statusEl.textContent = '已断开，正在重连...';
        // 自动重连
        setTimeout(initWebSocket, 3000);
    };

    // 连接错误
    ws.onerror = (error) => {
        console.error('WebSocket error:', error);
        statusEl.className = 'status offline';
        statusEl.textContent = '连接失败';
    };
}

// 渲染屏幕帧 base64
function renderScreenFrame(frame) {
    // 更新屏幕尺寸
    screenWidth = frame.width;
    screenHeight = frame.height;

    // 创建图片并渲染到Canvas
    const img = new Image();
    img.onload = () => {
        // 设置Canvas尺寸
        canvas.width = img.width;
        canvas.height = img.height;
        // 绘制图片
        ctx.drawImage(img, 0, 0);
    };
    img.src = `data:image/png;base64,${frame.base64}`;
}

// 渲染JPEG二进制帧
function renderJpegFrame(blob, meta) {
    // 创建图片URL
    const imgUrl = URL.createObjectURL(blob);
    const img = new Image();

    img.onload = () => {
        // 释放URL，避免内存泄漏
        URL.revokeObjectURL(imgUrl);

        // 设置Canvas尺寸
        canvas.width = meta.width;
        canvas.height = meta.height;

        // 绘制图片
        ctx.drawImage(img, 0, 0);
    };

    img.onerror = (err) => {
        console.error('Failed to load JPEG frame:', err);
        URL.revokeObjectURL(imgUrl);
    };

    img.src = imgUrl;
}


// 发送鼠标事件
function sendMouseEvent(eventData) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(eventData));
    }
}

// 发送键盘事件
function sendKeyEvent(eventData) {
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(eventData));
    }
}
// 重置键盘状态
function resetKeyState() {
    keyState.ctrl = false;
    keyState.shift = false;
    keyState.alt = false;
    keyState.meta = false;
    keyState.pressedKeys.clear();
}

// 格式化按键名称（统一命名规则）
function formatKeyName(key) {
    // 特殊按键映射
    const keyMap = {
        ' ': 'space',
        'Enter': 'enter',
        'Tab': 'tab',
        'Backspace': 'backspace',
        'Delete': 'delete',
        'Escape': 'esc',
        'ArrowUp': 'arrowup',
        'ArrowDown': 'arrowdown',
        'ArrowLeft': 'arrowleft',
        'ArrowRight': 'arrowright',
        'Home': 'home',
        'End': 'end',
        'PageUp': 'pageup',
        'PageDown': 'pagedown',
        'CapsLock': 'capslock',
        'Shift': 'shift',
        'Control': 'ctrl',
        'Alt': 'alt',
        'Meta': 'meta',
        'F1': 'f1', 'F2': 'f2', 'F3': 'f3', 'F4': 'f4', 'F5': 'f5',
        'F6': 'f6', 'F7': 'f7', 'F8': 'f8', 'F9': 'f9', 'F10': 'f10',
        'F11': 'f11', 'F12': 'f12',
        // 符号键映射（原始按键）
        '!': '1',
        '@': '2',
        '#': '3',
        '$': '4',
        '%': '5',
        '^': '6',
        '&': '7',
        '*': '8',
        '(': '9',
        ')': '0',
        '-': 'minus',
        '_': 'minus',
        '=': 'equal',
        '+': 'equal',
        '[': 'bracketleft',
        '{': 'bracketleft',
        ']': 'bracketright',
        '}': 'bracketright',
        '\\': 'backslash',
        '|': 'backslash',
        ';': 'semicolon',
        ':': 'semicolon',
        '\'': 'apostrophe',
        '"': 'apostrophe',
        ',': 'comma',
        '<': 'comma',
        '.': 'period',
        '>': 'period',
        '/': 'slash',
        '?': 'slash',
        '`': 'grave',
        '~': 'grave'
    };

    // 返回映射后的名称，无映射则转为小写
    return keyMap[key] || key.toLowerCase();
}

// 模拟单个按键的按下+释放
function simulateKeyPress(key) {
   const formattedKey = formatKeyName(key);

   // 构造按下事件
   const pressEvent = {
       type: 'keyboard',
       action: 'press',
       key: formattedKey,
       modifiers: {
           ctrl: false,
           shift: false,
           alt: false,
           meta: false
       }
   };

   // 构造释放事件
   const releaseEvent = {
       type: 'keyboard',
       action: 'release',
       key: formattedKey,
       modifiers: {
           ctrl: false,
           shift: false,
           alt: false,
           meta: false
       }
   };

   // 发送按下事件
   sendKeyEvent(pressEvent);
   console.log('模拟按键按下:', pressEvent);

   // 延迟发送释放事件（模拟真实按键）
   setTimeout(() => {
       sendKeyEvent(releaseEvent);
       console.log('模拟按键释放:', releaseEvent);
   }, 100);
}

// 模拟组合快捷键（如alt+tab、win+d）
function simulateShortcut(shortcut) {
   const keys = shortcut.split('+');
   const formattedKeys = keys.map(k => formatKeyName(k));

   // 1. 按下所有组合键（按顺序）
   formattedKeys.forEach((key, index) => {
       setTimeout(() => {
           sendKeyEvent({
               type: 'keyboard',
               action: 'press',
               key: key,
               modifiers: getModifiersFromKeys(formattedKeys.slice(0, index+1))
           });
           console.log(`模拟组合键按下: ${key}`);
       }, index * 50);
   });

   // 2. 延迟后释放所有组合键（逆序）
   setTimeout(() => {
       formattedKeys.reverse().forEach((key, index) => {
           setTimeout(() => {
               sendKeyEvent({
                   type: 'keyboard',
                   action: 'release',
                   key: key,
                   modifiers: getModifiersFromKeys(formattedKeys.slice(index+1).reverse())
               });
               console.log(`模拟组合键释放: ${key}`);
           }, index * 50);
       });
   }, formattedKeys.length * 50 + 100);
}

// 根据组合键列表生成modifiers对象
function getModifiersFromKeys(keys) {
   return {
       ctrl: keys.includes('ctrl'),
       shift: keys.includes('shift'),
       alt: keys.includes('alt'),
       meta: keys.includes('meta') || keys.includes('win')
   };
}

// 初始化系统按键面板事件
function initKeyButtonEvents() {
   // 单个按键按钮
   document.querySelectorAll('[data-key]').forEach(btn => {
       btn.addEventListener('click', () => {
           const key = btn.dataset.key;
           simulateKeyPress(key);
       });
   });

   // 快捷键组合按钮
   document.querySelectorAll('[data-shortcut]').forEach(btn => {
       btn.addEventListener('click', () => {
           const shortcut = btn.dataset.shortcut;
           simulateShortcut(shortcut);
       });
   });
}

// 监听Canvas鼠标事件
function initMouseEvents() {
    // 鼠标移动
    canvas.addEventListener('mousemove', (e) => {
    // 第一步：修正Canvas像素尺寸（关键！）
       const rect = canvas.getBoundingClientRect();
        // 打印核心数据：CSS显示尺寸、Canvas像素尺寸、鼠标原始坐标
//       console.log('CSS显示尺寸：', rect.width, rect.height);
//       console.log('Canvas像素尺寸：', canvas.width, canvas.height);
//       console.log('鼠标相对CSS坐标：', e.clientX - rect.left, e.clientY - rect.top);
       const scaleX = canvas.width / rect.width;
       const scaleY = canvas.height / rect.height;

       // 最终坐标（此时scaleX/Y=1，等价于 e.clientX - rect.left）
       const x = Math.floor((e.clientX - rect.left) * scaleX);
       const y = Math.floor((e.clientY - rect.top) * scaleY);

       sendMouseEvent({
           type: 'mouse_move',
           x: x,
           y: y,
           screen_width: canvas.width,  // 现在是正确的1920
           screen_height: canvas.height // 现在是正确的968
       });
//       console.log(`修正后坐标：${x} , ${y}  wh= ${canvas.width} x ${canvas.height}`);
    });

    // 鼠标按下
    canvas.addEventListener('mousedown', (e) => {
        let button = 'left';

        if (e.button === 2) { // 右键
            button = 'right';
            e.preventDefault(); // 阻止默认右键菜单
        } else if (e.button === 1) { // 中键/滚轮键
            button = 'middle';
            e.preventDefault(); // 阻止默认中键行为（如滚动）
        }

        sendMouseEvent({
            type: 'mouse_click',
            button: button,
            action: 'press'
        });
    });


    // 鼠标释放
    canvas.addEventListener('mouseup', (e) => {
        let button = 'left';
        if (e.button === 2) {
            button = 'right';
        }else if (e.button === 1) { // 中键/滚轮键
            button = 'middle';
        }

        sendMouseEvent({
            type: 'mouse_click',
            button: button,
            action: 'release'
        });
    });

    // 新增：鼠标滚轮事件
    canvas.addEventListener('wheel', (e) => {
       e.preventDefault(); // 阻止页面默认滚动行为
       // 判断滚轮方向（deltaY：向下为正，向上为负；deltaX：向右为正，向左为负）
       let direction = '';
       if (e.deltaY > 0) {
           direction = 'down'; // 向下滚动
       } else if (e.deltaY < 0) {
           direction = 'up';   // 向上滚动
       } else if (e.deltaX > 0) {
           direction = 'right';// 向右滚动
       } else if (e.deltaX < 0) {
           direction = 'left'; // 向左滚动
       }

       // 发送滚轮事件（包含方向+当前鼠标坐标）
       const rect = canvas.getBoundingClientRect();
       const scaleX = canvas.width / rect.width;
       const scaleY = canvas.height / rect.height;
       const x = Math.floor((e.clientX - rect.left) * scaleX);
       const y = Math.floor((e.clientY - rect.top) * scaleY);

       sendMouseEvent({
           type: 'mouse_wheel',       // 事件类型：滚轮
           direction: direction,      // 滚动方向：up/down/left/right
           steps: Math.abs(Math.floor(e.deltaY / 100) || Math.floor(e.deltaX / 100) || 1), // 滚动步数（标准化）
           x: x,                      // 滚轮触发时的鼠标X坐标
           y: y                       // 滚轮触发时的鼠标Y坐标
       });
       console.log(`滚轮滚动：${direction}，步数：${Math.abs(Math.floor(e.deltaY / 100) || 1)}，坐标：${x},${y}`);
    });

    // 阻止Canvas右键菜单
    canvas.addEventListener('contextmenu', (e) => {
        e.preventDefault();
    });
}

// 初始化键盘监听
function initKeyboardEvents() {
    // 定义需要排除的"关闭标签页"快捷键规则
    function isCloseTabShortcut(e) {
        // 匹配：Ctrl+W（Windows/Linux） 或 Cmd+W（Mac）
        const isWKey = e.key.toLowerCase() === 'w' || e.code === 'KeyW';
        const hasCtrlOrMeta = e.ctrlKey || e.metaKey; // Ctrl（Win/Linux）或 Meta（Mac）
        // 仅当 「Ctrl/Cmd + W」 组合时判定为关闭标签页快捷键
        return isWKey && hasCtrlOrMeta && !e.shiftKey && !e.altKey;
    }
    // 键盘按下事件
    document.addEventListener('keydown', (e) => {
        // 第一步：判断是否是关闭标签页快捷键，若是则阻止默认行为并直接返回（不发送事件）
         if (isCloseTabShortcut(e)) {
             e.preventDefault(); // 阻止关闭标签页
             console.log('已拦截关闭标签页快捷键：', e.key, '+', e.ctrlKey ? 'Ctrl' : 'Meta');
             return; // 不发送该事件到远程设备
         }
        // 阻止默认行为（如Ctrl+R刷新、F5刷新等）
        // 可根据需要调整需要阻止的组合键
        const preventDefaultKeys = [
            'F5', 'F11', 'Escape', 'Enter', 'Tab',
            'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'
        ];

        // 阻止特殊按键默认行为
        if (preventDefaultKeys.includes(e.key) || e.ctrlKey) {
            e.preventDefault();
        }

        // 更新组合键状态
        keyState.ctrl = e.ctrlKey || keyState.ctrl;
        keyState.shift = e.shiftKey || keyState.shift;
        keyState.alt = e.altKey || keyState.alt;
        keyState.meta = e.metaKey || keyState.meta;

        // 格式化按键名称
        const key = formatKeyName(e.key);

        // 避免重复发送相同按键的按下事件
        if (!keyState.pressedKeys.has(key)) {
            keyState.pressedKeys.add(key);

            // 构造键盘事件数据
            const keyEvent = {
                type: 'keyboard',
                action: 'press',
                key: key, // 标准化按键（如!转为1）
                // 组合键状态
                modifiers: {
                    ctrl: keyState.ctrl,
                    shift: keyState.shift, // 关键：保留Shift状态，后端根据Shift+1识别!
                    alt: keyState.alt,
                    meta: keyState.meta
                },
                // 原始按键信息（备用）
                rawKey: e.key,
                keyCode: e.keyCode || e.code
            };

            // 发送键盘按下事件
            sendKeyEvent(keyEvent);
            console.log('按键按下:', keyEvent);
        }
    });

    // 键盘释放事件
    document.addEventListener('keyup', (e) => {
        // 判断是否是关闭标签页快捷键，若是则直接返回（不发送释放事件）
         if (isCloseTabShortcut(e)) {
             console.log('已拦截关闭标签页快捷键释放事件：', e.key);
             // 重置对应状态（避免keyState残留）
             keyState.pressedKeys.delete(formatKeyName(e.key));
             keyState.ctrl = e.ctrlKey;
             keyState.meta = e.metaKey;
             return;
         }
        // 更新组合键状态
        keyState.ctrl = e.ctrlKey;
        keyState.shift = e.shiftKey;
        keyState.alt = e.altKey;
        keyState.meta = e.metaKey;

        // 格式化按键名称
        const key = formatKeyName(e.key);

        // 从已按下集合中移除
        keyState.pressedKeys.delete(key);

        // 构造键盘事件数据
        const keyEvent = {
            type: 'keyboard',
            action: 'release',
            key: key,
            // 组合键状态
            modifiers: {
                ctrl: keyState.ctrl,
                shift: keyState.shift,
                alt: keyState.alt,
                meta: keyState.meta
            },
            // 原始按键信息（备用）
            rawKey: e.key,
            keyCode: e.keyCode || e.code
        };

        // 发送键盘释放事件
        sendKeyEvent(keyEvent);
        console.log('按键释放:', keyEvent);
    });

    // 窗口失去焦点时重置键盘状态
    window.addEventListener('blur', () => {
        // 发送所有已按下按键的释放事件（排除关闭标签页快捷键    相关）
        keyState.pressedKeys.forEach(key => {
            // 仅跳过w键的释放（避免残留的关闭标签页快捷键状态）
            if (key !== 'w') {
                sendKeyEvent({
                    type: 'keyboard',
                    action: 'release',
                    key: key,
                    modifiers: {
                        ctrl: false,
                        shift: false,
                        alt: false,
                        meta: false
                    }
                });
            }
        });
        // 重置状态
        resetKeyState();
    });
}

// 页面加载完成初始化
window.onload = () => {
    initWebSocket();
    initMouseEvents();
    initKeyboardEvents(); // 初始化键盘监听
    initKeyButtonEvents(); // 初始化快捷键
};

// 页面关闭时清理连接
window.onbeforeunload = () => {
    if (ws) {
        ws.close();
    }
    // 重置键盘状态
    resetKeyState();
    // 清理所有未释放的Blob URL
    URL.revokeObjectURL = URL.revokeObjectURL || function() {};
};
