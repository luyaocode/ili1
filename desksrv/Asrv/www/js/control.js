// 1. 左侧垂直菜单的显示/隐藏逻辑
const sidebar = document.getElementById('sidebar');
const sidebarToggle = document.getElementById('sidebarToggle');
sidebarToggle.addEventListener('click', () => {
    sidebar.classList.toggle('show');
});

// 2. 面板比例调整逻辑（滑块+拖拽）
const ratioSlider = document.getElementById('ratioSlider');
const ratioText = document.getElementById('ratioText');
const screenPanel = document.getElementById('screen-panel');
const bashPanel = document.getElementById('bash-panel');
const panelSplitter = document.getElementById('panelSplitter');
const panelContainer = document.getElementById('panelContainer');

// 更新面板比例的函数
function updatePanelRatio(screenRatio, bashRatio) {
    screenPanel.style.flex = screenRatio;
    bashPanel.style.flex = bashRatio;
    ratioText.textContent = `${screenRatio}:${bashRatio}`;
    ratioSlider.value = screenRatio;
}

// 滑块调整比例
ratioSlider.addEventListener('input', () => {
    const screenRatio = parseInt(ratioSlider.value);
    const bashRatio = 10 - screenRatio; // 总比例和为10，范围1:9到9:1
    updatePanelRatio(screenRatio, bashRatio);
});

// 拖拽分隔条调整比例
let isDragging = false;
panelSplitter.addEventListener('mousedown', (e) => {
    isDragging = true;
    // 隐藏鼠标样式，提升拖拽体验
    document.body.style.cursor = 'col-resize';
    e.preventDefault();
});

document.addEventListener('mousemove', (e) => {
    if (!isDragging) return;

    // 获取容器的宽度和鼠标在容器内的位置
    const containerRect = panelContainer.getBoundingClientRect();
    const containerWidth = containerRect.width;
    const mouseX = e.clientX - containerRect.left;

    // 计算屏幕面板的比例（限制在10%~90%之间）
    const screenPercent = Math.max(10, Math.min(90, (mouseX / containerWidth) * 100));
    const screenRatio = Math.round(screenPercent / 10); // 转为1-9的整数
    const bashRatio = 10 - screenRatio;

    updatePanelRatio(screenRatio, bashRatio);
});

document.addEventListener('mouseup', () => {
    if (isDragging) {
        isDragging = false;
        document.body.style.cursor = 'default';
    }
});

// 3. 新增：通知发送逻辑
const notifyInput = document.getElementById('notify-input');
const notifyBtn = document.getElementById('notify-btn');

// 发送通知的核心函数
function sendNotify() {
    // 获取并去除首尾空格
    const text = notifyInput.value.trim();
    // 验证内容不为空且不超过50字符（input已加maxlength，这里做双重验证）
    if (!text) {
        alert('请输入通知内容！');
        return;
    }
    if (text.length > 50) {
        alert('通知内容不能超过50个字符！');
        return;
    }

    // 构造请求地址，编码特殊字符避免URL错误
    const url = `/$$notify/${encodeURIComponent(text)}`;
    // 发送GET请求（可根据需求改为POST）
    fetch(url)
        .then(response => {
            if (!response.ok) {
                throw new Error('请求失败');
            }
            // 清空输入框
            notifyInput.value = '';
            alert('通知发送成功！');
        })
        .catch(error => {
            console.error('发送通知失败：', error);
            alert('通知发送失败，请重试！');
        });
}

// 按钮点击发送
notifyBtn.addEventListener('click', sendNotify);

// 回车发送（绑定到输入框的keydown事件）
notifyInput.addEventListener('keydown', (e) => {
    if (e.key === 'Enter') {
        sendNotify();
    }
});
