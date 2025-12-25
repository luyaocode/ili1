#include "mainwindow.cpp"
#include <QApplication>
#include <rtc_base/thread.h>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    // 启动WebRTC线程（必须在主线程外）
    rtc::ThreadManager::Instance()->WrapCurrentThread();
    MainWindow w;
    w.show();
    return a.exec();
}
