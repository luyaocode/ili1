#include "app.h"
#include <QDebug>
#include "globaldefine.h"

App::App(): m_monitor(new Monitor(&m_inputWatcher, this)), m_eyesProtector(new EyesProtector(&m_inputWatcher, this))
{
}

ExtProcess &App::getReminder()
{
    return m_reminder;
}

bool App::run()
{
    if (!m_inputWatcher.startWatching())
    {
        qCritical() << "[App] "
                    << "无法启动输入监控";
        return false;
    }
    return m_monitor->start() && m_eyesProtector->start();
}
