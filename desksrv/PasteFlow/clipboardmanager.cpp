#include "clipboardmanager.h"
#include <QGuiApplication>
#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QAction>
#include <QMessageBox>
#include <QMimeData>
#include <QUrl>
#include <QEvent>
#include <QDebug>
#include <QTimer>
#include <QMimeData>
#include <QWidgetAction>
#include <QVBoxLayout>
#include <QLabel>
#include <QX11Info>

#include <X11/keysym.h>

#include "keyblocker.h"
#include "globaltool.h"
#include "x11struct.h"

ClipboardManager::ClipboardManager(QObject *parent): QObject(parent)
{
    // 初始化剪贴板
    m_clipboard = QGuiApplication::clipboard();

    // x11struct
    m_x11struct = new x11struct;

    // 初始化系统托盘
    m_trayIcon.reset(new QSystemTrayIcon(QIcon(":/icons/clipboard.png"), this));  // 需添加图标资源
    m_trayIcon->setToolTip("剪贴板队列");
    m_trayIcon->show();

    // 初始化托盘菜单
    QMenu   *trayMenu        = new QMenu;
    QAction *showQueueAction = new QAction("显示剪贴板队列", this);
    connect(showQueueAction, &QAction::triggered, this, &ClipboardManager::showQueueMenu);
    QAction *clearAction = new QAction("清空队列", this);
    connect(clearAction, &QAction::triggered, this, &ClipboardManager::clearQueue);
    QAction *exitAction = new QAction("退出", this);
    connect(exitAction, &QAction::triggered, qApp, &QApplication::quit);

    trayMenu->addAction(showQueueAction);
    trayMenu->addAction(clearAction);
    trayMenu->addSeparator();
    trayMenu->addAction(exitAction);
    m_trayIcon->setContextMenu(trayMenu);

    // 加载历史队列
    loadQueue();

    // 初始化全局快捷键（Ctrl+Shift+X）
    m_Blocker.reset(new KeyBlocker(
        {KeyWithModifier(XK_X, ControlMask | ShiftMask)}));
    connect(m_Blocker.data(), &KeyBlocker::sigBlocked, this, &ClipboardManager::showQueueMenu);
    m_Blocker->start();
}

ClipboardManager::~ClipboardManager()
{
    if (m_x11struct)
    {
        delete m_x11struct;
        m_x11struct = nullptr;
    }
}
// 开始监听剪贴板变化
void ClipboardManager::startMonitoring()
{
    connect(m_clipboard, &QClipboard::dataChanged, this, &ClipboardManager::onClipboardChanged);
}

// 剪贴板内容变化时触发
void ClipboardManager::onClipboardChanged()
{
    const QMimeData *mimeData = m_clipboard->mimeData();
    QString          content;
    bool             isFile = false;

    // 优先处理文件（如果复制的是文件）
    if (mimeData->hasUrls() && !mimeData->urls().isEmpty())
    {
        content = mimeData->urls().first().toLocalFile();  // 取第一个文件路径
        isFile  = true;
    }
    // 处理文本
    else if (mimeData->hasText())
    {
        content = mimeData->text().trimmed();
        isFile  = false;
    }
    else
    {
        return;  // 忽略其他类型（如图片，可后续扩展）
    }

    // 去重：如果和上一次内容相同则跳过
    if (content == m_lastContent)
        return;
    // 如果选择粘贴，就跳过
    if (m_btnSelPaste)
    {
        m_btnSelPaste = false;
        return;
    }

    // 添加到队列
    ClipboardItem item {.content = content, .timestamp = QDateTime::currentDateTime(), .isFile = isFile};
    m_queue.enqueue(item);

    // 限制队列长度
    while (m_queue.size() > m_maxSize)
    {
        m_queue.dequeue();  // 移除最早的项
    }

    m_lastContent = content;
    //    m_trayIcon->showMessage("剪贴板队列", "已添加新内容", QSystemTrayIcon::Information, 2000);

    // 保存队列
    saveQueue();
}
// 显示队列菜单（供用户选择粘贴项）
void ClipboardManager::showQueueMenu()
{
    m_queueMenu.reset(new QMenu("剪贴板历史"));

    if (m_queue.isEmpty())
    {
        QAction *emptyAction = new QAction("队列为空", m_queueMenu.data());
        emptyAction->setEnabled(false);
        m_queueMenu->addAction(emptyAction);
    }
    else
    {
        // 倒序显示（最新的在最上面）
        QList<ClipboardItem> reversedList;
        foreach (const ClipboardItem &item, m_queue)
        {  // 遍历队列中的所有元素
            reversedList.append(item);
        }
        std::reverse(reversedList.begin(), reversedList.end());  // 倒序排列

        for (const auto &item : reversedList)
        {
            QString displayText = item.content;
            // 长文本截断显示
            if (displayText.length() > 30)
            {
                displayText = displayText.left(30) + "...";
            }
            // 显示时间和内容
            QString actionText = QString("[%1] %2").arg(item.timestamp.toString("MM-dd HH:mm"), displayText);

            // 用 QWidgetAction 自定义菜单项
            QWidgetAction *widgetAction = new QWidgetAction(m_queueMenu.data());
            QWidget       *actionWidget = new QWidget();
            // 关键1：给承载控件启用 hover 事件，确保鼠标事件被捕获
            actionWidget->setAttribute(Qt::WA_Hover, true);
            QVBoxLayout *layout = new QVBoxLayout(actionWidget);
            layout->setContentsMargins(0, 0, 0, 0);  // 去掉多余内边距，扩大hover区域
            layout->setSpacing(0);

            // 自定义标签（核心：双重提示保障）
            QLabel *actionLabel = new QLabel(actionText);
            actionLabel->setCursor(Qt::PointingHandCursor);
            // 关键2：给标签启用 hover 事件+强制设置提示
            actionLabel->setAttribute(Qt::WA_Hover, true);
            actionLabel->setAttribute(Qt::WA_AlwaysShowToolTips);  // 强制显示提示，忽略系统设置
            actionLabel->setStyleSheet(R"(
                   QLabel {
                       color: #333;
                       padding: 6px 32px; /* 与菜单项内边距保持一致，加宽显示区域 */
                       white-space: nowrap; /* 强制单行显示 */
                   }
                   QLabel:hover {
                       color: #1a73e8;
                   }
               )");
            QString toolTip = item.isFile ? QString("文件路径：%1").arg(item.content) : item.content;
            actionLabel->setToolTip(toolTip);
            // 关键3：给 QWidgetAction 也设提示（双重保障，避免标签事件失效）
            widgetAction->setToolTip(toolTip);

            layout->addWidget(actionLabel);
            widgetAction->setDefaultWidget(actionWidget);
            actionWidget->setStyleSheet(R"(
                   QWidget:hover {
                       background-color: #e0eaf1; /* 与菜单选中色保持一致 */
                   }
               )");

            // 存储原始内容
            widgetAction->setProperty("content", item.content);
            widgetAction->setProperty("isFile", item.isFile);
            // 绑定点击事件
            connect(widgetAction, &QWidgetAction::triggered, this, &ClipboardManager::pasteSelectedItem);
            m_queueMenu->addAction(widgetAction);
        }

        m_queueMenu->addSeparator();
        QAction *clearAction = new QAction("清空队列", m_queueMenu.data());
        connect(clearAction, &QAction::triggered, this, &ClipboardManager::clearQueue);
        m_queueMenu->addAction(clearAction);
    }
    // 菜单基础样式设置（增加最小宽度）
    m_queueMenu->setStyleSheet(R"(
        QMenu {
            background-color: #f5f5f5;
            border: 1px solid #ddd;
            border-radius: 4px;
            padding: 4px 0;
            font-family: "Microsoft YaHei", sans-serif;
            font-size: 12px;
            min-width: 400px; /* 增大菜单最小宽度 */
        }
        QMenu::item {
            padding: 6px 32px; /* 增加左右内边距，加宽项目 */
            color: #333;
            white-space: nowrap; /* 强制单行显示 */
        }
        QMenu::item:selected {
            background-color: #e0eaf1;
            color: #1a73e8;
        }
        QMenu::separator {
            height: 1px;
            background-color: #ddd;
            margin: 4px 0;
        }
    )");
    // 在鼠标位置显示菜单
    m_queueMenu->exec(QCursor::pos());
}
// 粘贴选中的项到剪贴板并触发粘贴（模拟 Ctrl+V）
void ClipboardManager::pasteSelectedItem()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action)
        return;

    QString content = action->property("content").toString();
    bool    isFile  = action->property("isFile").toBool();
    // 创建新的 MimeData
    m_lastMimeData = new QMimeData;
    if (isFile)
    {
        // 处理文件：添加标准 URL + XDND 协议数据
        QList<QUrl> fileUrls = {QUrl::fromLocalFile(content)};
        m_lastMimeData->setUrls(fileUrls);
    }
    else
    {
        m_lastMimeData->setText(content);
    }

    m_btnSelPaste = true;

    // 关键：同时设置 X11 的 PRIMARY 和 CLIPBOARD 选择器
    // 1. 设置 CLIPBOARD（默认）
    m_clipboard->setMimeData(m_lastMimeData, QClipboard::Clipboard);
    // 2. 设置 PRIMARY（鼠标选中的剪贴板，大多数应用 Ctrl+V 会读取）
    // 注意：clone() 避免所有权冲突，因为 setMimeData 会接管所有权
    QMimeData *selectionMimeData = copyMimeData(m_lastMimeData);
    m_clipboard->setMimeData(selectionMimeData, QClipboard::Selection);
    activateX11PrimarySelection(selectionMimeData);
    // 模拟 Ctrl+V（增加延时，确保剪贴板数据已同步）
    // 在 pasteSelectedItem 中调用
    // 延迟 50ms，确保剪贴板数据同步和焦点稳定
    QTimer::singleShot(50, [this]() {
        bool                isTerminal = isTerminalWindowX11();
        std::vector<KeySym> maskKeys;
        if (isTerminal)
        {
            maskKeys.push_back(XK_Control_L);
            maskKeys.push_back(XK_Shift_L);
        }
        else
        {
            maskKeys.push_back(XK_Control_L);
        }
        KeySym targetKey = XK_V;
        simulateKeyWithMask(maskKeys, targetKey);
    });
}

// 清空队列
void ClipboardManager::clearQueue()
{
    m_queue.clear();
    m_lastContent.clear();
    saveQueue();
    //    m_trayIcon->showMessage("剪贴板队列", "已清空队列", QSystemTrayIcon::Information, 1000);
}

// 保存队列到文件（持久化）
void ClipboardManager::saveQueue()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/clipboard_queue.json";
    QDir().mkpath(QFileInfo(path).path());  // 创建目录

    QJsonArray jsonArray;
    for (const auto &item : m_queue)
    {
        QJsonObject obj;
        obj["content"]   = item.content;
        obj["timestamp"] = item.timestamp.toString(Qt::ISODate);
        obj["isFile"]    = item.isFile;
        jsonArray.append(obj);
    }

    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(QJsonDocument(jsonArray).toJson());
    }
}
// 从文件加载队列
void ClipboardManager::loadQueue()
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/clipboard_queue.json";
    QFile   file(path);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonArray jsonArray = QJsonDocument::fromJson(file.readAll()).array();
    for (const auto &json : jsonArray)
    {
        QJsonObject   obj = json.toObject();
        ClipboardItem item;
        item.content   = obj["content"].toString();
        item.timestamp = QDateTime::fromString(obj["timestamp"].toString(), Qt::ISODate);
        item.isFile    = obj["isFile"].toBool();
        m_queue.enqueue(item);
    }

    // 初始化最后一条内容（用于去重）
    if (!m_queue.isEmpty())
    {
        m_lastContent = m_queue.last().content;
    }
}

// 检查内容是否为文件路径（简化版）
bool ClipboardManager::isFilePath(const QString &content)
{
    return QFileInfo::exists(content) && QFileInfo(content).isFile();
}

// 手动复制 QMimeData 的所有数据（兼容 Qt 5.15 以下版本）
QMimeData *ClipboardManager::copyMimeData(const QMimeData *source)
{
    if (!source)
        return nullptr;

    QMimeData *dest = new QMimeData;

    // 1. 复制文本数据（补充 X11 原生格式，兼容终端/记事本）
    if (source->hasText())
    {
        QString text = source->text();
        dest->setText(text);
        // 关键：添加 X11 原生 "COMPOUND_TEXT" 格式（终端/记事本优先识别）
        dest->setData("COMPOUND_TEXT", text.toLocal8Bit());
        // 补充 "UTF8_STRING" 格式（现代 X11 应用支持）
        dest->setData("UTF8_STRING", text.toUtf8());
    }

    // 2. 复制 URL 数据（文件粘贴，终端不支持，记事本支持）
    if (source->hasUrls())
    {
        dest->setUrls(source->urls());
        // 补充 URL 的原生格式（text/uri-list 是标准，但部分应用需要）
        QStringList uriList;
        for (const QUrl &url : source->urls())
        {
            uriList.append(url.toString());
        }
        dest->setData("text/uri-list", uriList.join("\n").toUtf8());
    }

    // 3. 其他 MIME 类型（保持不变）
    const QStringList mimeTypes = source->formats();
    for (const QString &mimeType : mimeTypes)
    {
        // 只过滤文本相关的重复格式，保留文件相关格式
        if (mimeType == "text/plain" || mimeType == "COMPOUND_TEXT" || mimeType == "UTF8_STRING")
        {
            continue;
        }
        // 保留 text/uri-list、x-special/gnome-copied-files 等
        dest->setData(mimeType, source->data(mimeType));
    }

    return dest;
}
