#include <QCoreApplication>
#include <QThread>
#include <QDebug>
#include <QTimer>
#include <chrono>
#include <thread>
#include "dispatcher.h"

// 模拟DBus调用：获取电源板版本（带参数+返回值）
QString mockGetPowerBoardVersion(const QString &boardName, int slotId)
{
    // 模拟实际DBus阻塞调用
    std::this_thread::sleep_for(std::chrono::seconds(2));
    return QString("V1.2.3-%1-Slot%2").arg(boardName).arg(slotId);
}

// 模拟DBus调用：设置电源板参数（无返回值）
void mockSetPowerBoardParam(const QString &boardName, int voltage, bool enable)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    qDebug() << "[Mock DBus] 设置" << boardName << "电压：" << voltage << "使能：" << enable;
}

// 模拟DBus调用：获取温度（单个参数+返回值）
double mockGetTemperature(int sensorId)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 25.6 + sensorId * 0.5;  // 模拟不同传感器温度
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    qDebug() << "[主线程]" << QThread::currentThreadId() << "启动";

    // 1. 启动事件分发器线程
    DispatcherThread dispatcher;
    dispatcher.start();

    // 2. 测试1：调用带参数+返回值的DBus方法（获取电源板版本）
    qDebug() << "\n[业务线程] 调用获取电源板版本DBus接口（阻塞等待）";
    std::future<QString> versionFuture =
        dispatcher.callDbusInterface<QString, QString, int>(mockGetPowerBoardVersion, "PowerBoard-A", 1, 3000);
    try
    {
        QString version = versionFuture.get();
        qDebug() << "[业务线程] 电源板版本获取成功：" << version;
    }
    catch (const std::exception &e)
    {
        qDebug() << "[业务线程] 电源板版本获取失败：" << e.what();
    }

    // 3. 测试2：调用无返回值的DBus方法（设置电源板参数）
    qDebug() << "\n[业务线程] 调用设置电源板参数DBus接口（阻塞等待）";
    std::future<void> setParamFuture =
        dispatcher.callDbusInterface<QString, int, bool>(mockSetPowerBoardParam, "PowerBoard-A", 12, true, 3000);
    try
    {
        setParamFuture.get();
        qDebug() << "[业务线程] 电源板参数设置完成";
    }
    catch (const std::exception &e)
    {
        qDebug() << "[业务线程] 电源板参数设置失败：" << e.what();
    }

    // 4. 测试3：调用单个参数+返回值的DBus方法（获取温度）
    qDebug() << "\n[业务线程] 调用获取温度DBus接口（阻塞等待）";
    std::future<double> tempFuture = dispatcher.callDbusInterface<double, int>(mockGetTemperature, 3, 3000);
    try
    {
        double temp = tempFuture.get();
        qDebug() << "[业务线程] 传感器温度获取成功：" << temp << "℃";
    }
    catch (const std::exception &e)
    {
        qDebug() << "[业务线程] 传感器温度获取失败：" << e.what();
    }

    qDebug() << "\n[业务线程]" << QThread::currentThreadId() << "执行结束";

    // 延迟退出
    QTimer::singleShot(1000, &a, &QCoreApplication::quit);

    return a.exec();
}
