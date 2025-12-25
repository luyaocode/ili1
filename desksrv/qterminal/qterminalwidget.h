#ifndef QTERMINALWIDGET_H
#define QTERMINALWIDGET_H

#include <QWidget>
#include <QPlainTextEdit>
#include <QKeyEvent>
#include <QStringList>
#include <QSocketNotifier>
#include <pty.h>   // 伪终端相关（Linux）
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

class QTerminalWidget : public QWidget
{
    Q_OBJECT

public:
    explicit QTerminalWidget(QWidget *parent = nullptr);
    ~QTerminalWidget() override;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onPtyReadyRead();               // 处理伪终端输出
    void onWindowResize();               // 窗口大小变化时通知终端

private:
    QPlainTextEdit *m_terminal;          // 显示控件
    QString m_currentInput;              // 当前输入命令
    QStringList m_commandHistory;        // 命令历史
    int m_historyIndex;                  // 历史记录索引
    int m_ptyMasterFd;                   // 伪终端主设备描述符
    pid_t m_childPid;                    // bash进程PID
    QSocketNotifier *m_ptyNotifier;      // 监听伪终端输入

    QString parseAnsiEscapeCodes(const QByteArray &data);  // 解析ANSI序列
    void sendCommand(const QString &command);               // 发送命令到终端
    void updateHistory(const QString &command);             // 更新命令历史
    void setTerminalSize();                                 // 设置终端窗口大小
};

#endif // QTERMINALWIDGET_H
