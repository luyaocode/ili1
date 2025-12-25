#include "qterminalwidget.h"
#include <QVBoxLayout>
#include <QFont>
#include <QRegExp>
#include <QTextCursor>
#include <QDebug>
#include <QResizeEvent>

QTerminalWidget::QTerminalWidget(QWidget *parent)
    : QWidget(parent)
    , m_terminal(new QPlainTextEdit)
    , m_historyIndex(-1)
    , m_ptyMasterFd(-1)
    , m_childPid(-1)
    , m_ptyNotifier(nullptr)
{
    // 布局设置
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_terminal);

    // 终端显示设置
    m_terminal->setReadOnly(true);
    m_terminal->setFont(QFont("Consolas", 10));  // 等宽字体
    m_terminal->setStyleSheet("background-color: black; color: white;");
    m_terminal->installEventFilter(this);        // 捕获键盘输入

    // 创建伪终端并启动bash
    char slaveName[256];
    m_childPid = forkpty(&m_ptyMasterFd, slaveName, nullptr, nullptr);
    if (m_childPid < 0) {
        qCritical() << "创建伪终端失败!";
        return;
    }

    if (m_childPid == 0) {
        // 子进程：启动交互式bash
        execlp("/bin/bash", "bash", "-i", nullptr);
        _exit(1);  // 如果execlp失败，退出子进程
    }

    // 监听伪终端输出
    m_ptyNotifier = new QSocketNotifier(m_ptyMasterFd, QSocketNotifier::Read, this);
    connect(m_ptyNotifier, &QSocketNotifier::activated, this, &QTerminalWidget::onPtyReadyRead);

    // 关键：禁用伪终端的本地回显（在父进程中配置）
    struct termios ttyAttr;
    if (tcgetattr(m_ptyMasterFd, &ttyAttr) != -1) {
       ttyAttr.c_lflag &= ~ECHO;  // 清除ECHO标志，禁用回显
       ttyAttr.c_lflag &= ~ECHOE; // 可选：禁用退格回显（避免删除时显示）
       ttyAttr.c_lflag &= ~ECHOK; // 可选：禁用杀死行回显
       ttyAttr.c_lflag &= ~ECHONL; // 可选：禁用换行回显
       tcsetattr(m_ptyMasterFd, TCSANOW, &ttyAttr); // 立即生效
    }

    // 窗口大小变化时通知终端
    connect(this, &QTerminalWidget::windowTitleChanged, this, &QTerminalWidget::onWindowResize);

    // 初始设置终端大小
    setTerminalSize();
}

QTerminalWidget::~QTerminalWidget()
{
    // 清理资源
    if (m_childPid > 0) {
        kill(m_childPid, SIGTERM);  // 终止bash进程
        waitpid(m_childPid, nullptr, 0);
    }
    if (m_ptyMasterFd != -1) {
        ::close(m_ptyMasterFd);  // 关闭伪终端
    }
    delete m_ptyNotifier;
}

// 处理伪终端输出
void QTerminalWidget::onPtyReadyRead()
{
    char buffer[1024];
    ssize_t bytesRead = read(m_ptyMasterFd, buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        QByteArray output(buffer, bytesRead);
        QString parsed = parseAnsiEscapeCodes(output);
        m_terminal->insertPlainText(parsed);
        m_terminal->moveCursor(QTextCursor::End);  // 自动滚动到底部
    }
}

// 窗口大小变化时更新终端尺寸
void QTerminalWidget::onWindowResize()
{
    setTerminalSize();
}

// 设置终端窗口大小（行和列）
void QTerminalWidget::setTerminalSize()
{
    if (m_ptyMasterFd == -1) return;

    // 获取终端显示区域的字符行数和列数
    QFontMetrics fm(m_terminal->font());
    int cols = m_terminal->width() / fm.averageCharWidth();
    int rows = m_terminal->height() / fm.height();

    if (cols > 0 && rows > 0) {
        // 构造终端大小控制序列
        struct winsize ws;
        ws.ws_col = cols;    // 列数
        ws.ws_row = rows;    // 行数
        ws.ws_xpixel = 0;
        ws.ws_ypixel = 0;
        ioctl(m_ptyMasterFd, TIOCSWINSZ, &ws);  // 通知终端大小变化
    }
}

// 解析ANSI转义序列（颜色、清屏等）
QString QTerminalWidget::parseAnsiEscapeCodes(const QByteArray &data)
{
    QString text = QString::fromLocal8Bit(data);

    // 新增：过滤退格符（\b）
    text.remove('\b');  // 直接移除退格符，避免显示
    // 处理颜色和样式
    text.replace(QRegExp("\033\\[0m"), "</span>");                  // 重置样式
    text.replace(QRegExp("\033\\[31m"), "<span style='color: red;'>");    // 红色
    text.replace(QRegExp("\033\\[32m"), "<span style='color: green;'>");  // 绿色
    text.replace(QRegExp("\033\\[33m"), "<span style='color: yellow;'>"); // 黄色
    text.replace(QRegExp("\033\\[34m"), "<span style='color: blue;'>");   // 蓝色
    text.replace(QRegExp("\033\\[1m"), "<span style='font-weight: bold;'>"); // 加粗

    // 处理清屏命令
    if (text.contains(QRegExp("\033\\[2J"))) {
        m_terminal->clear();
        text.remove(QRegExp("\033\\[2J"));
    }

    // 处理光标移动到行首
    text.replace(QRegExp("\033\\[G"), "");

    return text;
}

// 发送命令到终端
void QTerminalWidget::sendCommand(const QString &command)
{
    if (m_ptyMasterFd == -1) return;

    if (!command.trimmed().isEmpty()) {
        updateHistory(command);
    }
    // 向伪终端写入命令（加换行符执行）
    QByteArray data = command.toLocal8Bit() + "\n";
    write(m_ptyMasterFd, data.data(), data.size());
}

// 更新命令历史（去重）
void QTerminalWidget::updateHistory(const QString &command)
{
    if (!m_commandHistory.isEmpty() && m_commandHistory.last() == command) {
        return;
    }
    m_commandHistory.append(command);
    m_historyIndex = m_commandHistory.size();  // 指向历史末尾
}

// 事件过滤器：处理键盘输入
bool QTerminalWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_terminal && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        // 处理Ctrl+C（发送中断信号）
        if (keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_C) {
            if (m_childPid > 0) {
                kill(m_childPid, SIGINT);
            }
            return true;
        }

        // 处理Ctrl+D（发送EOF）
        if (keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_D) {
            if (m_ptyMasterFd != -1) {
                write(m_ptyMasterFd, "\x04", 1);  // EOF的ASCII码
            }
            return true;
        }

        // 处理回车键（发送命令）
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            m_terminal->insertPlainText("\n");
            sendCommand(m_currentInput);
            m_currentInput.clear();
            m_historyIndex = m_commandHistory.size();
            return true;
        }

        // 处理退格键
        if (keyEvent->key() == Qt::Key_Backspace && !m_currentInput.isEmpty()) {
            m_currentInput.chop(1);
            QTextCursor cursor = m_terminal->textCursor();
            cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, 1);
            cursor.removeSelectedText();
            return true;
        }

        // 上箭头：历史命令上移
        if (keyEvent->key() == Qt::Key_Up) {
            if (m_historyIndex > 0) {
                m_historyIndex--;
                QTextCursor cursor = m_terminal->textCursor();
                cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
                cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                m_terminal->insertPlainText(m_commandHistory[m_historyIndex]);
                m_currentInput = m_commandHistory[m_historyIndex];
            }
            return true;
        }

        // 下箭头：历史命令下移
        if (keyEvent->key() == Qt::Key_Down) {
            if (m_historyIndex < m_commandHistory.size() - 1) {
                m_historyIndex++;
                QTextCursor cursor = m_terminal->textCursor();
                cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
                cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                m_terminal->insertPlainText(m_commandHistory[m_historyIndex]);
                m_currentInput = m_commandHistory[m_historyIndex];
            } else {
                QTextCursor cursor = m_terminal->textCursor();
                cursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
                cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                m_currentInput.clear();
                m_historyIndex = m_commandHistory.size();
            }
            return true;
        }

        // 普通字符输入
        QString text = keyEvent->text();
        if (!text.isEmpty()) {
            m_currentInput += text;
            m_terminal->insertPlainText(text);
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}
