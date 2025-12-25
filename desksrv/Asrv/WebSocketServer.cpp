#include "WebSocketServer.h"
#include <QDebug>
#include <QProcessEnvironment>
#include <QWebSocket>
#include <QByteArray>
#include <termios.h>
#include <pty.h>
#include <unistd.h>
#include <sys/wait.h>
#include <QThread>
#include <QFile>
#include <errno.h>   // 新增：错误码定义
#include <string.h>  // 新增：strerror 所需
#include <fcntl.h>   // 包含fcntl头文件

#include "def.h"
#include "tool.h"

// 初始化静态成员变量
QMap<QWebSocket *, int> WebSocketServer::m_clientPtyMap;

WebSocketServer::WebSocketServer(QObject *parent): QObject(parent)
{
}

WebSocketServer::~WebSocketServer()
{
    stopListen();
}

bool WebSocketServer::startListen(const QString &serverIp, quint16 serverPort)
{
    // 原有逻辑不变，保留
    if (m_wsServer)
    {
        stopListen();
    }

    m_wsServer = new QWebSocketServer("Independent WebSocket Server", QWebSocketServer::NonSecureMode, this);

    QHostAddress listenAddr;
    if (serverIp.isEmpty())
    {
        listenAddr = QHostAddress::Any;
    }
    else
    {
        if (!listenAddr.setAddress(serverIp))
        {
            qWarning() << "WebSocketServer: 无效的监听IP地址：" << serverIp;
            delete m_wsServer;
            m_wsServer = nullptr;
            return false;
        }
    }

    if (!m_wsServer->listen(listenAddr, serverPort))
    {
        qWarning() << "WebSocketServer: 启动监听失败：" << m_wsServer->errorString();
        //        emit errorOccurred(nullptr, QAbstractSocket::HostNotFoundError, m_wsServer->errorString());
        delete m_wsServer;
        m_wsServer = nullptr;
        return false;
    }

    m_listenIp   = serverIp;
    m_listenPort = serverPort;

    connect(m_wsServer, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
    connect(m_wsServer, &QWebSocketServer::closed, this, []() {
        qInfo() << "WebSocketServer: 监听已关闭";
    });

    qInfo() << "WebSocketServer: 启动成功，监听地址：" << (serverIp.isEmpty() ? "0.0.0.0" : serverIp) << "端口："
            << serverPort;

    return true;
}

void WebSocketServer::stopListen()
{
    if (m_wsServer)
    {
        // 关闭所有客户端连接、Shell 进程、PTY
        for (auto it = m_clientShellPidMap.begin(); it != m_clientShellPidMap.end(); ++it)
        {
            QWebSocket *client   = it.key();
            pid_t       shellPid = it.value();

            // 关闭 PTY
            if (m_clientPtyMap.contains(client))
            {
                close(m_clientPtyMap[client]);
                m_clientPtyMap.remove(client);
            }

            // 终止 Shell 进程（避免僵尸进程）
            if (shellPid > 0)
            {
                kill(shellPid, SIGTERM);
                waitpid(shellPid, nullptr, WNOHANG);
            }

            // 关闭客户端
            client->close();
            client->deleteLater();
        }

        m_clientShellPidMap.clear();
        m_clientPtyMap.clear();

        m_wsServer->close();
        delete m_wsServer;
        m_wsServer = nullptr;
    }
    qInfo() << "WebSocketServer: 停止监听";
}

// 核心修复：创建 PTY 并启动 Shell（使用 fork/exec，兼容 Qt 5.9）
bool WebSocketServer::createPtyAndStartShell(QWebSocket *client)
{
    int            ptyMasterFd = -1;
    int            ptySlaveFd  = -1;
    struct termios termAttr;
    struct winsize winSize = {24, 80, 0, 0};

    // 1. 创建 PTY（兼容无 termAttr 的情况，避免部分系统报错）
    if (openpty(&ptyMasterFd, &ptySlaveFd, nullptr, nullptr, &winSize) < 0)
    {
        qCritical() << "创建 PTY 失败：" << strerror(errno) << "（客户端：" << client->peerAddress().toString() << "）";
        return false;
    }
    // 2. 配置 PTY（支持交互式命令）
    tcgetattr(ptyMasterFd, &termAttr);

    // 设置非阻塞
    // 2. 配置 PTY 为非阻塞模式（避免 read 时阻塞）
    int flags = fcntl(ptyMasterFd, F_GETFL, 0);
    fcntl(ptyMasterFd, F_SETFL, flags | O_NONBLOCK);

    //    setTermAttr(termAttr);
    setTermAttrAsync(termAttr);
    //    tcgetattr(ptyMasterFd, &termAttr); // 获取 PTY 属性
    //    termAttr.c_lflag &= ~(ECHO | ICANON); // 关闭回显（ECHO）和规范模式（ICANON）
    //    tcsetattr(ptyMasterFd, TCSANOW, &termAttr); // 立即生效

    // 立即应用配置（TCSANOW：无需等待输入完成）
    if (tcsetattr(ptyMasterFd, TCSANOW, &termAttr) < 0)
    {
        qCritical() << "配置 PTY 失败：" << strerror(errno) << "（客户端：" << client->peerAddress().toString() << "）";
        close(ptyMasterFd);
        close(ptySlaveFd);
        return false;
    }

    // 3. 确认 Shell 绝对路径（解决 "No such file or directory"）
    QString shellPath;
    if (QFile::exists("/bin/bash"))
        shellPath = "/bin/bash";
    else if (QFile::exists("/usr/bin/bash"))
        shellPath = "/usr/bin/bash";
    else if (QFile::exists("/bin/sh"))
        shellPath = "/bin/sh";
    else if (QFile::exists("/usr/bin/sh"))
        shellPath = "/usr/bin/sh";
    else
    {
        qCritical() << "未找到 Shell（bash/sh）："
                    << "（客户端：" << client->peerAddress().toString() << "）";
        close(ptyMasterFd);
        close(ptySlaveFd);
        return false;
    }

    // 4. 手动 fork 创建子进程（Qt 5.9 核心兼容点）
    pid_t shellPid = fork();
    if (shellPid < 0)
    {
        qCritical() << "fork 子进程失败：" << strerror(errno) << "（客户端：" << client->peerAddress().toString()
                    << "）";
        close(ptyMasterFd);
        close(ptySlaveFd);
        return false;
    }
    else if (shellPid == 0)  // 子进程：启动 Shell 并绑定 PTY
    {
        // 1. 关键修复：创建新会话，脱离父进程控制终端（核心解决“不允许的操作”）
        if (setsid() < 0)
        {
            perror("setsid 创建新会话失败");
            _exit(EXIT_FAILURE);
        }

        // 2. 禁用 ssh-askpass（保留原有优化）
        unsetenv("DISPLAY");
        setenv("SSH_ASKPASS", "", 1);

        // 3. 重定向 stdin/stdout/stderr 到 PTY Slave FD（保留原有逻辑）
        if (dup2(ptySlaveFd, STDIN_FILENO) == -1 || dup2(ptySlaveFd, STDOUT_FILENO) == -1 ||
            dup2(ptySlaveFd, STDERR_FILENO) == -1)
        {
            perror("子进程 FD 重定向失败");
            _exit(EXIT_FAILURE);
        }

        // 4. 给 PTY Slave 分配终端名称并设置为控制终端（保留原有逻辑）
        char ptsName[256];
        if (ttyname_r(ptySlaveFd, ptsName, sizeof(ptsName)) != 0)
        {
            perror("获取 PTY 终端名称失败");
            _exit(EXIT_FAILURE);
        }
        // 现在子进程无控制终端，可正常设置 PTY 为控制终端
        if (ioctl(STDIN_FILENO, TIOCSCTTY, 1) < 0)
        {
            perror("设置控制终端失败");
            _exit(EXIT_FAILURE);
        }

        // 5. 设置终端类型环境变量（保留原有逻辑）
        setenv("TERM", "xterm", 1);

        // 6. 验证终端是否为交互式（可选，调试用）
        if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO) || !isatty(STDERR_FILENO))
        {
            perror("PTY 未被识别为交互式终端");
            _exit(EXIT_FAILURE);
        }

        // 7. 关闭无用 FD（保留原有逻辑）
        close(ptyMasterFd);
        close(ptySlaveFd);

        // 8. 执行 Shell（保留原有逻辑）
        const char *shellName = shellPath.split("/").last().toUtf8().data();
        execl(shellPath.toUtf8().data(), shellName, nullptr);

        perror(("启动 Shell 失败：" + shellPath).toUtf8().data());
        _exit(EXIT_FAILURE);
    }
    else  // 父进程：管理 PTY 和 Shell PID
    {
        // 关闭父进程的 PTY Slave FD（仅保留 Master 通信）
        close(ptySlaveFd);

        // 验证 Shell 进程是否启动成功（短暂延时检查）
        sleep(1);
        if (kill(shellPid, 0) < 0 && errno == ESRCH)
        {
            qCritical() << "Shell 进程启动失败（已退出）："
                        << "（客户端：" << client->peerAddress().toString() << "）";
            close(ptyMasterFd);
            return false;
        }

        // 保存 PTY FD 和 Shell PID（用于后续清理）
        m_clientPtyMap[client]      = ptyMasterFd;
        m_clientShellPidMap[client] = shellPid;
        qInfo() << "PTY 绑定成功（客户端：" << client->peerAddress().toString() << "）"
                << "，Shell：" << shellPath << "，PTY Master FD：" << ptyMasterFd << "，Shell PID：" << shellPid;

        return true;
    }
}

// 子线程读取 PTY 输出（原有逻辑不变，仅优化客户端状态判断）
void WebSocketServer::readPtyOutput(QWebSocket *client)
{
    if (!client || !m_clientPtyMap.contains(client))
        return;

    int     ptyMasterFd = m_clientPtyMap[client];
    char    buffer[1024];
    ssize_t bytesRead;
    while (true)
    {
        bytesRead = read(ptyMasterFd, buffer, sizeof(buffer) - 1);
        if (bytesRead > 0)
        {
            // 1. 读取 PTY 原始数据
            QByteArray output(buffer, bytesRead);
            // 关键调试：打印 read 返回值和读取到的原始数据
//            qInfo() << "PTY 读取结果：bytesRead=" << bytesRead
//                    << " errno=" << (bytesRead < 0 ? strerror(errno) : "无错误") << " 读取到的原始数据（hex）："
//                    << QByteArray(buffer, qMax(0, (int)bytesRead)).toHex();

            // 2. 编码转换（避免乱码）
            //        QString msg = QString::fromUtf8(output);

            //
            //            QString msg = QString::fromUtf8(filterAnsiEscapeCodes(output));

            // 3. 关键修复：跨线程委托主线程发送消息（避免操作 QSocketNotifier）
            // 使用 Qt::QueuedConnection 确保主线程执行
            //            bool invokeOk = QMetaObject::invokeMethod(this, "sendMessageToClient", Qt::QueuedConnection,
            //                                                      Q_ARG(QWebSocket *, client), Q_ARG(QString, msg));
            bool invokeOk = QMetaObject::invokeMethod(this, "sendBinaryToClient", Qt::QueuedConnection,
                                                      Q_ARG(QWebSocket *, client), Q_ARG(QByteArray, output));

            if (!invokeOk)
            {
                qWarning() << "委托主线程发送消息失败（客户端：" << client->peerAddress().toString() << "）";
            }
        }
        else if (bytesRead == 0)
        {
            qInfo() << "PTY 正常关闭（客户端：" << client->peerAddress().toString() << "）";
            break;  // 退出循环，清理资源
        }
        else
        {
            // 3. 读取失败：判断错误类型
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 关键修复：EAGAIN 是正常现象（无数据可读），忽略并继续
                QThread::msleep(10);  // 短暂延时（10ms），避免 CPU 空转
                continue;             // 跳过后续处理，回到循环继续读取
            }
            else
            {
                // 其他错误：真失败（如 FD 无效、PTY 断开）
                qWarning() << "PTY 读取真错误：" << strerror(errno) << "（客户端：" << client->peerAddress().toString()
                           << "，FD：" << ptyMasterFd << "）";
                break;  // 退出循环，清理资源
            }
        }
    }

    // 触发客户端断开和资源清理
    QMetaObject::invokeMethod(this, "closeClientConnection", Qt::QueuedConnection, Q_ARG(QWebSocket *, client));
}

// 新客户端连接（修改：移除 QProcess，直接调用修复后的 createPtyAndStartShell）
void WebSocketServer::onNewConnection()
{
    QWebSocket *clientSocket = m_wsServer->nextPendingConnection();
    if (!clientSocket)
    {
        qWarning() << "WebSocketServer: 获取待处理连接失败";
        return;
    }

    const QString clientAddr = clientSocket->peerAddress().toString();
    qInfo() << "WebSocketServer: 新客户端连接：" << clientAddr;

    // 连接客户端信号
    clientSocket->disconnect(this);
    //    connect(clientSocket, &QWebSocket::textMessageReceived, this, &WebSocketServer::onTextMessageReceived);
    connect(clientSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::onBinaryReceived);
    connect(clientSocket, &QWebSocket::disconnected, this, &WebSocketServer::onClientDisconnected);
    clientSocket->setParent(this);

    // 绑定 PTY 并启动 Shell（无需 QProcess）
    if (!createPtyAndStartShell(clientSocket))
    {
        const QString errMsg = "[错误] 无法创建伪终端或启动 Shell，连接即将关闭\n";
        clientSocket->sendTextMessage(errMsg);
        clientSocket->disconnect();
        clientSocket->close(QWebSocketProtocol::CloseCodeAbnormalDisconnection, "PTY创建失败");
        clientSocket->deleteLater();
        qWarning() << "WebSocketServer: 客户端" << clientAddr << "PTY创建失败，已关闭连接";
        return;
    }

    // 启动子线程读取 PTY 输出
    QThread *ptyReadThread          = new QThread(this);
    m_clientThreadMap[clientSocket] = ptyReadThread;
    connect(ptyReadThread, &QThread::started, [=]() {
        qInfo() << "WebSocketServer: 客户端" << clientAddr << "PTY读取线程启动";
        readPtyOutput(clientSocket);
        qInfo() << "WebSocketServer: 客户端" << clientAddr << "PTY读取线程退出循环";
    });
    connect(clientSocket, &QWebSocket::disconnected, [=]() {
        // 客户端断开时退出线程
        ptyReadThread->quit();
        ptyReadThread->wait(1000);
        ptyReadThread->deleteLater();
    });
    ptyReadThread->start();

    // 向客户端发送欢迎信息
    clientSocket->sendTextMessage("[WebSocket Bash (PTY模式)] 已连接（exit退出）\n");
}

// 客户端断开连接（修改：清理 Shell 进程和 PTY）
void WebSocketServer::onClientDisconnected()
{
    QWebSocket *clientSocket = qobject_cast<QWebSocket *>(sender());
    if (!clientSocket)
        return;

    qInfo() << "WebSocketServer: 客户端断开连接：" << clientSocket->peerAddress().toString();

    // ========== 1. 先关闭PTY（唤醒线程的阻塞IO） ==========
    if (m_clientPtyMap.contains(clientSocket))
    {
        int ptyFd = m_clientPtyMap[clientSocket];
        if (ptyFd >= 0)
        {
            if (close(ptyFd) == 0)
            {
                qInfo() << "PTY资源已释放（FD：" << ptyFd << "）";
            }
            else
            {
                qWarning() << "关闭PTY失败（FD：" << ptyFd << "），错误码：" << errno;
            }
        }
        m_clientPtyMap.remove(clientSocket);
    }

    // ========== 2. 再终止Shell进程（释放子进程资源） ==========
    if (m_clientShellPidMap.contains(clientSocket))
    {
        pid_t shellPid = m_clientShellPidMap[clientSocket];
        // 先发送SIGTERM优雅终止，失败则发送SIGKILL强制杀死
        if (kill(shellPid, SIGTERM) != 0)
        {
            qWarning() << "优雅终止Shell进程失败，强制杀死（PID：" << shellPid << "）";
            kill(shellPid, SIGKILL);
        }
        // 非阻塞回收子进程（避免waitpid阻塞主线程）
        int status = 0;
        waitpid(shellPid, &status, WNOHANG);
        qInfo() << "已终止Shell进程（PID：" << shellPid << "），退出状态：" << status;
        m_clientShellPidMap.remove(clientSocket);
    }

    // ========== 3. 最后停止并回收工作线程（此时线程已从阻塞中唤醒） ==========
    if (m_clientThreadMap.contains(clientSocket))
    {
        QThread *workerThread = m_clientThreadMap[clientSocket];
        if (workerThread && workerThread->isRunning())
        {
            // 向工作线程发送退出信号（需工作线程实现响应逻辑）
            //            QMetaObject::invokeMethod(workerThread->findChild<ShellWorker *>(), "onQuit", Qt::QueuedConnection);

            // 安全停止线程：先请求退出，再等待结束
            workerThread->quit();
            // 延长超时时间至2秒（给线程足够时间处理退出）
            if (!workerThread->wait(2000))
            {
                qWarning() << "线程退出超时，强制终止：" << workerThread;
                workerThread->terminate();
                workerThread->wait();
            }
        }
        workerThread->deleteLater();
        m_clientThreadMap.remove(clientSocket);
        qInfo() << "客户端关联线程已回收";
    }

    // ========== 4. 延迟释放客户端Socket ==========
    clientSocket->disconnect();
    clientSocket->deleteLater();
    qInfo() << "客户端Socket已标记为延迟释放";
}

// 接收客户端输入（原有逻辑不变，保留）
void WebSocketServer::onTextMessageReceived(const QString &message)
{
    QWebSocket *clientSocket = qobject_cast<QWebSocket *>(sender());
    if (!clientSocket || !m_clientPtyMap.contains(clientSocket))
        return;

    qInfo() << "收到客户端指令：" << message << "（客户端：" << clientSocket->peerAddress().toString() << "）";

    // 处理退出指令
    if (message.trimmed().toLower() == "exit")
    {
        // 终止 Shell 进程
        if (m_clientShellPidMap.contains(clientSocket))
        {
            pid_t shellPid = m_clientShellPidMap[clientSocket];
            kill(shellPid, SIGTERM);
            waitpid(shellPid, nullptr, WNOHANG);
        }
        clientSocket->sendTextMessage("[WebSocket Bash] 已退出Shell，连接即将关闭\n");
        clientSocket->close();
        return;
    }

    // 将输入写入 PTY（触发 Shell 执行命令）
    int        ptyMasterFd = m_clientPtyMap[clientSocket];
    QByteArray input       = message.toUtf8() + "\n";  // 加换行符提交命令
    ssize_t    writeLen    = write(ptyMasterFd, input.data(), input.size());
    if (writeLen != input.size())
    {
        qWarning() << "PTY 写入失败：" << strerror(errno) << "（客户端：" << clientSocket->peerAddress().toString()
                   << "）";
        clientSocket->sendTextMessage("[错误] 命令发送失败\n");
    }
}

void WebSocketServer::onBinaryReceived(const QByteArray &binary)
{
    QWebSocket *clientSocket = qobject_cast<QWebSocket *>(sender());
    if (!clientSocket || !m_clientPtyMap.contains(clientSocket))
        return;

    qInfo() << "收到客户端指令：bin " << binary << "（客户端：" << clientSocket->peerAddress().toString() << "）";

    // 处理退出指令
    if (QString(binary).toLower() == "exit")
    {
        // 终止 Shell 进程
        if (m_clientShellPidMap.contains(clientSocket))
        {
            pid_t shellPid = m_clientShellPidMap[clientSocket];
            kill(shellPid, SIGTERM);
            waitpid(shellPid, nullptr, WNOHANG);
        }
        clientSocket->sendTextMessage("[WebSocket Bash] 已退出Shell，连接即将关闭\n");
        clientSocket->close();
        return;
    }

    // 将输入写入 PTY（触发 Shell 执行命令）
    int ptyMasterFd = m_clientPtyMap[clientSocket];
    //    QByteArray input       = message.toUtf8() + "\n";  // 加换行符提交命令
    QByteArray input    = binary + '\n';
    ssize_t    writeLen = write(ptyMasterFd, input.data(), input.size());
    if (writeLen != input.size())
    {
        qWarning() << "PTY 写入失败：" << strerror(errno) << "（客户端：" << clientSocket->peerAddress().toString()
                   << "）";
        clientSocket->sendTextMessage("[错误] 命令发送失败\n");
    }
}

// 进程结束处理（移除 QProcess 相关逻辑，改为空实现即可）
void WebSocketServer::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode);
    Q_UNUSED(exitStatus);
    // 不再使用 QProcess，此函数无实际作用，保留以避免编译错误
}

// 关闭客户端连接（原有逻辑不变，保留）
void WebSocketServer::closeClientConnection(QWebSocket *client)
{
    if (client && client->state() == QAbstractSocket::ConnectedState)
    {
        client->close();
    }
}

void WebSocketServer::sendMessageToClient(QWebSocket *client, const QString &msg)
{
    if (!client)
        return;

    // 主线程中验证连接状态并发送
    if (client->state() == QAbstractSocket::ConnectedState)
    {
        qint64 sentLen = client->sendTextMessage(msg);
        if (sentLen <= 0)
        {
            qWarning() << "主线程发送消息失败（客户端：" << client->peerAddress().toString() << "），错误："
                       << client->errorString();
        }
        else
        {
            qDebug().noquote() << "[主线程→客户端]（发送长度：" << sentLen << "）\n" << msg;
        }
    }
    else
    {
        qWarning() << "客户端已断开，跳过发送（客户端：" << client->peerAddress().toString() << "）";
    }
}

void WebSocketServer::sendBinaryToClient(QWebSocket *client, const QByteArray &binary)
{
    if (!client)
        return;

    // 主线程中验证连接状态并发送
    if (client->state() == QAbstractSocket::ConnectedState)
    {
        qint64 sentLen = client->sendBinaryMessage(binary);
        if (sentLen <= 0)
        {
            qWarning() << "主线程发送消息失败（客户端：" << client->peerAddress().toString() << "），错误："
                       << client->errorString();
        }
        else
        {
            qDebug().noquote() << "[主线程→客户端] bin（发送长度：" << sentLen << "）\n" << binary;
        }
    }
    else
    {
        qWarning() << "客户端已断开，跳过发送（客户端：" << client->peerAddress().toString() << "）";
    }
}
