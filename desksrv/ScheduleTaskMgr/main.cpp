#include <QApplication>
#include <QDebug>
#include "scheduletaskmgr.h"

#include "commontool/globaltool.h"
#include "commontool/keyblocker.h"
#include "def.h"

int main(int argc, char *argv[])
{
    if (isAlreadyRunning(APP_NAME))
    {
        return 1;
    }
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);
    qRegisterMetaType<KeyWithModifier>("KeyWithModifier");
    qRegisterMetaType<std::vector<KeyWithModifier>>("std::vector<KeyWithModifier>");
    ScheduleTaskMgr mgr;
    return a.exec();
}
