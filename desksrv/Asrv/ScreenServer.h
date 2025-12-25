#ifndef SCREENSERVER_H
#define SCREENSERVER_H
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QMap>
#include <QProcess>
#include <QTimer>
#include <QPixmap>
#include <QPainter>
#include <QJsonDocument>
#include <QJsonObject>
#include <sys/types.h>
#include <QScreen>
#include <QBuffer>
#include <unordered_map>

#include "commontool/mousesimulator.h"
#include "ClientInfo.h"

class ScreenServer : public QObject
{
    Q_OBJECT
public:
    explicit ScreenServer(QObject *parent = nullptr);
    ~ScreenServer();

    bool startListen(const QString &serverIp, quint16 serverPort);
    void stopListen();

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onTextMessageReceived(const QString &message);
    void onBinaryReceived(const QByteArray &binary);
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void closeClientConnection(QWebSocket *client);
    void sendMessageToClient(QWebSocket *client, const QString &msg);
    void sendBinaryToClient(QWebSocket *client, const QByteArray &binary);
    // 新增：定时截屏并推送
    void captureScreenAndPush();
    // 新增：处理鼠标事件
    void handleMouseEvent(const QJsonObject &mouseEvent, QWebSocket *client);
    void handleKeyboardEvent(const QJsonObject &mouseEvent, QWebSocket *client);

private:
    QWebSocketServer              *m_wsServer = nullptr;
    QString                        m_listenIp;
    quint16                        m_listenPort   = 0;
    QTimer                        *m_captureTimer = nullptr;       // 截屏定时器
    QMap<QWebSocket *, ClientInfo> m_clientMap;                    // 客户端映射
    QProcess                      *m_screenshotProcess = nullptr;  // 截屏进程
private:
    void                           getRealXY(const ClientInfo &info, int &x, int &y);
    MouseSimulator::WheelDirection getScrollWhellDirection(const QString &direction);
};

#endif  // SCREENSERVER_H
