#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QMap>
#include <QTimer>
#include <QMutex>
#include "def.h"

class TcpServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit TcpServer(QObject *parent = nullptr);
    void startListen(const QString &serverIp, const quint16 serverPort);

protected:
    void incomingConnection(qintptr socketDescriptor) override;
private slots:
    void onSocketDisconnected();  // 新增：客户端断开时清理缓存
    // 停止推流（清理定时器和socket关联）
    void slotStopScreenStream(QTcpSocket *socket);

private:
    // 处理客户端HTTP请求
    void handleClientRequest(QTcpSocket *socket);
    /**
     * 处理静态资源请求
     * @param requestPath 请求的路径（如 /js/file_browser.js、/css/style.css）
     * @param socket 客户端socket
     * @return bool 是否为静态资源请求并处理成功
     */
    bool handleStaticResource(const QString &requestPath, QTcpSocket *socket);
    // 处理文件上传请求
    void handleUploadRequest(QTcpSocket *socket, const QByteArray &requestData);
    bool checkUploadDataComplete(QTcpSocket *socket, const QByteArray &requestData);
    void doHandleUploadRequest(QTcpSocket *socket, const QByteArray &requestData);
    // 处理特殊请求
    bool handleScreenRequest(const QString &requestPath, QTcpSocket *socket);
    bool handleRTC(const QString &requestPath, QTcpSocket *socket);
    void stopRTC(QTcpSocket *socket);
    // 通知
    bool handleNotify(const QString &requestPath, QTcpSocket *socket);
    void showNotifyPopup(const QString &message);
    // 测试页面
    bool handleTest(const QString &requestPath, QTcpSocket *socket);
    // bash页面
    bool handleBash(const QString &requestPath, QTcpSocket *socket);
    // handleControl
    bool handleControl(const QString &requestPath, QTcpSocket *socket);
    // xterm
    bool handleXterm(const QString &requestPath, QTcpSocket *socket);
    // 远程控制
    bool handleScreenCtrl(const QString &requestPath, QTcpSocket *socket);

private:
    QString                        m_listenIp;
    quint16                        m_listenPort = 0;
    QString                        m_rootDir;
    QMap<QTcpSocket *, QByteArray> m_socketRequestCache;  // 每个socket的请求数据缓存

    // handleRTC
    QMap<QTcpSocket *, QTimer *> m_screenStreamTimers;  // 推流定时器映射
    QMutex                       m_timerMutex;          // 定时器操作互斥锁
};

#endif  // TCPSERVER_H
