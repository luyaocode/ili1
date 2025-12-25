#ifndef WEBSOCKETSERVER_H
#define WEBSOCKETSERVER_H

#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QMap>
#include <QProcess>
#include <sys/types.h>  // 新增：pid_t 所需头文件

class WebSocketServer : public QObject
{
    Q_OBJECT
public:
    explicit WebSocketServer(QObject *parent = nullptr);
    ~WebSocketServer();

    bool startListen(const QString &serverIp, quint16 serverPort);
    void stopListen();

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryReceived(const QByteArray &binary);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void closeClientConnection(QWebSocket *client);  // 已存在，保留
    // 新增：主线程执行的消息发送槽函数（参数：客户端 + 要发送的字符串）
    void sendMessageToClient(QWebSocket *client, const QString &msg);
    void sendBinaryToClient(QWebSocket *client, const QByteArray &binary);

private:
    bool createPtyAndStartShell(QWebSocket *client);  // 修改：移除 QProcess 参数
    void readPtyOutput(QWebSocket *client);

    QWebSocketServer *m_wsServer = nullptr;
    QString           m_listenIp;
    quint16           m_listenPort = 0;

    static QMap<QWebSocket *, int> m_clientPtyMap;       // 客户端 → PTY Master FD（已存在）
    QMap<QWebSocket *, pid_t>      m_clientShellPidMap;  // 新增：客户端 → Shell 进程 PID
    QMap<QWebSocket *, QThread *>  m_clientThreadMap;  // 客户端-工作线程映射（新增：管理通信线程）
};

#endif  // WEBSOCKETSERVER_H
