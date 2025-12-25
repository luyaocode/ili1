#ifndef CLIPBOARDMANAGER_H
#define CLIPBOARDMANAGER_H

#include <QObject>
#include <QClipboard>
#include <QQueue>
#include <QDateTime>
#include <QSystemTrayIcon>
#include <QMenu>

struct x11struct;
struct KeyWithModifier;
class KeyBlocker;

// 剪贴板队列项结构体
struct ClipboardItem
{
    QString   content;    // 内容（文本或文件路径）
    QDateTime timestamp;  // 复制时间
    bool      isFile;     // 是否为文件（true=文件路径，false=文本）
};

class ClipboardManager : public QObject
{
    Q_OBJECT
public:
    explicit ClipboardManager(QObject *parent = nullptr);
    ~ClipboardManager();  // 必须加上,否则编译器默认的析构函数会检查KeyBlocker的完整定义,导致编译失败
    void startMonitoring();  // 开始监听剪贴板

private slots:
    void onClipboardChanged();                       // 剪贴板变化时触发
    void showQueueMenu();  // 显示队列菜单
    void pasteSelectedItem();                        // 粘贴选中的项
    void clearQueue();                               // 清空队列
    void saveQueue();                                // 保存队列到文件
    void loadQueue();                                // 从文件加载队列
private:
    QMimeData *copyMimeData(const QMimeData *source);
    // 检查内容是否为文件路径
    bool isFilePath(const QString &content);

private:
    QClipboard *m_clipboard;     // Qt 剪贴板对象
    x11struct  *m_x11struct;     // X11 剪贴板窗口句柄（关键）
    QMimeData  *m_lastMimeData;  // 剪切板set后自动管理QMimeData,再次set后会析构之前的QMimeData
    QQueue<ClipboardItem>           m_queue;         // 剪贴板队列
    int                             m_maxSize = 20;  // 队列最大长度
    QString                         m_lastContent;   // 上一次内容（用于去重）
    QScopedPointer<QSystemTrayIcon> m_trayIcon;      // 系统托盘图标
    QScopedPointer<QMenu>           m_queueMenu;     // 队列菜单
    QScopedPointer<KeyBlocker>      m_Blocker;
    bool                            m_btnSelPaste = false;  // 选择右键菜单进行粘贴
};

#endif  // CLIPBOARDMANAGER_H
