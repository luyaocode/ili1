#include <QCoreApplication>
#include "MockProp.h"

QList<MockProp *> propList;
bool isCleaning = false;
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // 定时器1：每1ms插入一个新对象（高频干扰）
    QTimer insertTimer;
    insertTimer.setInterval(1);
    QObject::connect(&insertTimer, &QTimer::timeout, insertNewProp);
    insertTimer.start();

    // 定时器2：每3ms执行一次清理（模拟20秒清理）
    QTimer cleanupTimer;
    cleanupTimer.setInterval(3);
    QObject::connect(&cleanupTimer, &QTimer::timeout, cleanupPropList);
    cleanupTimer.start();

    // 运行1秒后退出（足够触发double free）
    //    QTimer::singleShot(1000, &a, &QCoreApplication::quit);

    return a.exec();
}
