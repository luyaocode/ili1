#include <QApplication>
#include <QDebug>
#include "quickmanager.h"

#include "commontool/globaltool.h"
#include "commontool/keyblocker.h"
#include "unify/asynctaskrunner.hpp"

Q_DECLARE_METATYPE(unify::EM_TASK_STATE)

int main(int argc, char *argv[])
{
    if (isAlreadyRunning("QuickManager"))
    {
        qDebug() << "QuickManager is already running.";
        return 1;
    }
    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);  // 关键：禁用「最后一个窗口关闭时退出应用」的默认行为
    qRegisterMetaType<KeyWithModifier>("KeyWithModifier");
    qRegisterMetaType<std::vector<KeyWithModifier>>("std::vector<KeyWithModifier>");
    qRegisterMetaType<unify::EM_TASK_STATE>("unify::EM_TASK_STATE");
    QuickManager mgr;
    return a.exec();
}
