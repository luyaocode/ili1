// signalingclient.cpp
#include "signalingclient.h"
#include <QDebug>

SignalingClient::SignalingClient(const QUrl& serverUrl, QObject* parent)
    : QObject(parent), m_serverUrl(serverUrl) {
    connect(&m_webSocket, &QWebSocket::connected, this, &SignalingClient::onConnected);
    connect(&m_webSocket, &QWebSocket::textMessageReceived,
            this, &SignalingClient::onTextMessageReceived);
    m_webSocket.open(QUrl(serverUrl));
}

void SignalingClient::sendMessage(const std::string& msg) {
    if (m_webSocket.state() == QAbstractSocket::ConnectedState) {
        m_webSocket.sendTextMessage(QString::fromStdString(msg));
    } else {
        qWarning() << "WebSocket not connected, cannot send message";
    }
}

void SignalingClient::onConnected() {
    qDebug() << "Connected to signaling server";
}

void SignalingClient::onTextMessageReceived(const QString& message) {
    emit messageReceived(message.toStdString());
}
