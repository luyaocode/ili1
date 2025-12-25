#ifndef SERVER_H
#define SERVER_H
#include "TcpServer.h"
#include "WebSocketServer.h"
#include "ScreenServer.h"
class Server
{
public:
    explicit Server(QString ip = IP_ADDR, quint16 port = IP_PORT);
    static quint16 getWsPort();
    static quint16 getScreenServerPort();

private:
    TcpServer       m_tcpSrv;
    WebSocketServer m_wsSrv;
    ScreenServer    m_screenSrv;
};

#endif  // SERVER_H
