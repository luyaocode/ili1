// signaling_client.h
#ifndef SIGNALING_CLIENT_H
#define SIGNALING_CLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QUrl>

class SignalingClient : public QObject {
    Q_OBJECT
public:
    explicit SignalingClient(const QUrl& serverUrl, QObject* parent = nullptr);
    void sendMessage(const std::string& msg);

signals:
    void messageReceived(const std::string& msg); // 收到信令消息，转发给WebRTC模块

private slots:
    void onConnected();
    void onTextMessageReceived(const QString& message);

private:
    QWebSocket m_webSocket;
    QUrl m_serverUrl;
};

#endif // SIGNALING_CLIENT_H
