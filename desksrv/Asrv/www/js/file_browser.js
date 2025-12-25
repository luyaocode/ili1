// 全局元素缓存
const filePanel = document.getElementById('filePanel');
const previewPanel = document.getElementById('previewPanel');
const fileUploadInput = document.getElementById('fileUploadInput'); // 新增：文件选择框
const uploadBtn = document.querySelector('.upload-btn'); // 新增：上传按钮

// 初始化：绑定所有文件项事件
document.addEventListener('DOMContentLoaded', function() {
    bindFileItemEvents();
    bindDownloadBtnEvents(); // 新增：绑定下载按钮事件
    bindUploadBtnEvents(); // 新增：绑定上传按钮事件
});

/**
 * 处理中文路径的 URL 编码
 * @param {string} filePath 原始文件路径（含中文）
 * @returns {string} 编码后的路径
 */
function encodeFilePath(filePath) {
    // 注意：encodeURIComponent 会编码所有特殊字符（包括 /），若路径包含 /，需拆分处理
    const pathSegments = filePath.split('/');
    const encodedSegments = pathSegments.map(segment => {
        // 对每个分段进行编码（保留 / 作为路径分隔符）
        return encodeURIComponent(segment);
    });
    // 拼接回完整路径（将 / 还原）
    return encodedSegments.join('/');
}


/**
 * 绑定文件/目录项点击事件
 */
function bindFileItemEvents() {
    const fileItems = document.querySelectorAll('.file-item');
    fileItems.forEach(item => {
        const isDir = item.classList.contains('dir-item');
        const fileLink = item.querySelector('a');
        const filePath = item.dataset.filePath;

        // 阻止事件冒泡（避免冲突）
        item.addEventListener('click', function(e) {
            e.stopPropagation();
        });

        // 目录项：保留原生a标签跳转（不拦截）
        if (isDir) {
            return;
        }

        // 文件项：拦截a标签默认跳转，改为预览
        if (fileLink && filePath) {
            fileLink.addEventListener('click', async function(e) {
                // 阻止a标签原生跳转
                e.preventDefault();
                // 执行文件预览逻辑
                await previewFile(encodeFilePath(filePath));
            });
        }
    });
}

/**
 * 新增：绑定下载按钮点击事件
 */
function bindDownloadBtnEvents() {
    const downloadBtns = document.querySelectorAll('.download-btn');
    downloadBtns.forEach(btn => {
        btn.addEventListener('click', function(e) {
            // 阻止事件冒泡（避免和预览/收起逻辑冲突）
            e.stopPropagation();

            // 获取按钮上的文件路径（从data-file-path属性）
            const filePath = this.dataset.filePath;
            if (!filePath) {
                alert('文件路径不存在！');
                return;
            }

            // 编码中文路径，生成可访问的URL
            const encodedPath = encodeFilePath(filePath);

            // 方式1：创建临时a标签触发下载（推荐，支持指定下载文件名）
            const a = document.createElement('a');
            a.href = encodedPath;
            // 可选：指定下载文件名（若不设置，浏览器会用默认文件名）
            // a.download = filePath.split('/').pop(); // 提取路径最后一段作为文件名
            a.target = '_blank'; // 新标签页打开（可选，根据需求调整）
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);

            // 方式2：直接跳转（简单版，若不需要指定下载名可使用）
            // window.location.href = encodedPath;
            // 或新标签页打开：window.open(encodedPath, '_blank');
        });
    });
}

/**
 * 新增：绑定上传按钮事件
 */
function bindUploadBtnEvents() {
    // 点击上传按钮，触发文件选择框的点击
    uploadBtn.addEventListener('click', function() {
        fileUploadInput.click();
    });

    // 选择文件后，触发上传逻辑
    fileUploadInput.addEventListener('change', function() {
        const files = this.files;
        if (files.length === 0) {
            return; // 未选择文件，直接返回
        }
        uploadFiles(files);
    });
}

/**
 * 新增：上传文件到服务端
 * @param {FileList} files 选择的文件列表
 */
async function uploadFiles(files) {
    // 调整超时时间（先临时调大，比如10分钟，排查是否是超时导致）
    const UPLOAD_TIMEOUT = 10 * 60 * 1000; // 临时设为10分钟
    const controller = new AbortController();
    const timeoutTimer = setTimeout(() => {
        controller.abort();
        console.log('【主动超时】请求超过10分钟，已中断'); // 新增：打印主动超时日志
    }, UPLOAD_TIMEOUT);

    const formData = new FormData();
    for (let i = 0; i < files.length; i++) {
        formData.append('upload_files', files[i]);
        // 新增：打印文件大小，确认是否是超大文件
        console.log(`文件${i}大小：${(files[i].size / 1024 / 1024).toFixed(2)}MB`);
    }

    try {
        console.log('开始上传，请求地址：/upload');
        const response = await fetch('/upload', {
            method: 'POST',
            body: formData,
            signal: controller.signal,
            // 新增：禁用浏览器默认的请求缓存，强制刷新
            cache: 'no-store',
            // 新增：延长请求保持连接时间（部分浏览器生效）
            headers: {
                'Connection': 'keep-alive',
                'Keep-Alive': 'timeout=600' // 10分钟
            }
        });

        clearTimeout(timeoutTimer);
        if (!response.ok) {
            throw new Error(`上传失败：${response.status} ${response.statusText}`);
        }

        const result = await response.json();
        if (result.success) {
            showUploadTip('文件上传成功！', 'success');
            setTimeout(() => window.location.reload(), 1000);
        } else {
            throw new Error(result.message || '上传失败');
        }
    } catch (err) {
        clearTimeout(timeoutTimer);
        // 精准区分错误类型
        console.error('错误详情：', err);
        console.error('错误名称：', err.name);
        console.error('错误堆栈：', err.stack);

        if (err.name === 'AbortError') {
            showUploadTip('文件上传超时（超过10分钟）！请检查网络或减小文件大小', 'error');
        } else if (err.message.includes('Failed to fetch')) {
            // 连接被中止，大概率是服务端/网络问题
            showUploadTip('上传连接被中断！可能是文件过大或服务端限制，请联系管理员', 'error');
        } else {
            showUploadTip(err.message, 'error');
        }
    }
}

/**
 * 新增：显示上传提示框
 * @param {string} message 提示信息
 * @param {string} type 类型（success/error）
 */
function showUploadTip(message, type) {
    // 检查是否已存在提示框，若存在则移除
    const existingTip = document.querySelector('.upload-tip');
    if (existingTip) {
        existingTip.remove();
    }

    // 创建提示框元素
    const tip = document.createElement('div');
    tip.className = `upload-tip upload-${type}`;
    tip.textContent = message;
    document.body.appendChild(tip);

    // 显示提示框
    setTimeout(() => {
        tip.classList.add('show');
    }, 10);

    // 3秒后隐藏并移除提示框
    setTimeout(() => {
        tip.classList.remove('show');
        setTimeout(() => {
            tip.remove();
        }, 300);
    }, 3000);
}

/**
 * 预览文件核心逻辑
 * @param {string} filePath - 文件HTTP访问路径
 */
async function previewFile(filePath) {
    // 1. 展开预览区
    previewPanel.classList.add('show');
    filePanel.style.width = '40%';

    // 2. 显示加载状态
    previewPanel.innerHTML = '<div class="preview-placeholder">加载中...</div>';

    try {
        // 3. 请求文件内容
        // 直接请求原路径，通过请求头传递预览标识+密钥（避免路径冲突）
        const response = await fetch(filePath, {
           method: 'GET',
           headers: {
               // 预览标识
               'X-File-Action': 'preview',
               // 简单密钥（服务端验证，防止伪造请求）
               'X-Preview-Key': 'your_secret_key_123'
           }
        });
        if (!response.ok) {
            throw new Error(`HTTP错误：${response.status} ${response.statusText}`);
        }

        // 4. 解析MIME类型并渲染
        const contentType = response.headers.get('Content-Type') || '';
        const previewContent = document.createElement('div');

        if (isTextType(contentType)) {
            // 文本类文件：显示内容
            const content = await response.text();
            previewContent.className = 'preview-content';
            previewContent.textContent = content;
        } else {
            // 非文本文件：提示不支持
            previewContent.className = 'preview-unsupported';
            previewContent.innerHTML = `
                <h3>不支持预览该类型文件</h3>
                <p>文件类型：${contentType || '未知'}</p>
                <p>路径：${filePath}</p>
            `;
        }

        // 5. 更新预览区内容
        previewPanel.innerHTML = '';
        previewPanel.appendChild(previewContent);

    } catch (err) {
        // 6. 预览失败处理
        previewPanel.innerHTML = `
            <div class="preview-unsupported">
                <h3>预览失败</h3>
                <p>${err.message}</p>
                <p>文件路径：${filePath || '未知'}</p>
            </div>
        `;
        console.error('文件预览错误：', err);
    }
}

/**
 * 判断是否为文本类型（可预览）
 * @param {string} contentType - MIME类型
 * @returns {boolean}
 */
function isTextType(contentType) {
    const textTypes = [
        'text/', 'javascript', 'css', 'html', 'json',
        'application/x-javascript', 'application/json',
        'application/xml', 'text/xml'
    ];
    return textTypes.some(type => contentType.includes(type));
}

/**
 * 可选：点击空白处收起预览区
 */
document.addEventListener('click', function(e) {
    const isClickOnFileItem = e.target.closest('.file-item');
    const isClickOnPreview = e.target.closest('.preview-panel');

    if (!isClickOnFileItem && !isClickOnPreview) {
        previewPanel.classList.remove('show');
        filePanel.style.width = '100%';
        previewPanel.innerHTML = '<div class="preview-placeholder">点击左侧文件查看内容预览</div>';
    }
});
