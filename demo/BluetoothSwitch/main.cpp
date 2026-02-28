#include "MainWindow.h"
#include <QApplication>
#include "MainThreadBlockDetector.h"
#include <QBluetoothDeviceInfo>
#include <QSet>
#include <QLoggingCategory> // 必须包含该头文件
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//     方式3：同时启用warning/debug（确保qCWarning也能打印）
     QLoggingCategory::setFilterRules(R"(
         qt.bluetooth.bluez.debug = true
         qt.bluetooth.bluez.warning = true
     )");
    qRegisterMetaType<QSet<QBluetoothDeviceInfo>>("QSet<QBluetoothDeviceInfo>");
    MainThreadBlockDetector detector(50, 100);
    MainWindow w;
    w.show();

    return a.exec();
}
