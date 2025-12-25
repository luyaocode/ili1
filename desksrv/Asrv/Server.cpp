#include "Server.h"
#include <QFileInfo>
#include "def.h"
#include "tool.h"

static quint16 s_wsPort             = 0;
static quint16 s_wsScreenServerPort = 0;

Server::Server(QString ip, quint16 port)
{
    m_tcpSrv.startListen(ip, port);
    m_wsSrv.startListen(ip, port + 1);
    m_screenSrv.startListen(ip, port + 2);
    s_wsPort             = port + 1;
    s_wsScreenServerPort = port + 2;
}

quint16 Server::getWsPort()
{
    return s_wsPort;
}

quint16 Server::getScreenServerPort()
{
    return s_wsScreenServerPort;
}
