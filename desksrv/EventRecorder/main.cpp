#include <QCoreApplication>
#include "application_controller.h"
#include <iostream>
#include <csignal>   // 用于 signal() 和 SIGINT

// 全局控制器实例，用于信号处理
ApplicationController* g_controller = nullptr;

// 2. 定义信号处理函数
void signal_handler(int signal_num) {
    std::cout << "\n收到信号: " << signal_num << " (通常是 Ctrl+C)。" << std::endl;

    // 检查全局指针是否有效
    if (g_controller != nullptr) {
        // 在信号处理函数中调用 stopRecord()
        g_controller->stopRecord();
    } else {
        std::cout << "全局控制器指针为空，无法调用 stopRecord。" << std::endl;
    }

    // 注意：信号处理函数执行完毕后，程序会继续执行被中断的指令。
    // 如果希望程序在停止录制后退出，可以调用 exit()。
    // 但 exit() 可能不会调用全局对象的析构函数，需要谨慎。
    // 一个更好的方式是设置一个标志，让主循环优雅地退出。
    // 这里为了演示，我们直接退出。
    std::cout << "程序将在信号处理后退出。" << std::endl;
    exit(signal_num);
}


int main(int argc, char *argv[]) {
    // 必须在创建任何 Qt 对象之前创建 QCoreApplication
    QCoreApplication a(argc, argv);

    ApplicationController controller;
    g_controller = &controller;

    signal(SIGINT, signal_handler);
    // 将 controller 的 finished 信号连接到 app 的 quit 槽
    QObject::connect(&controller, &ApplicationController::finished, &a, &QCoreApplication::quit);

    // 启动控制器的初始化流程
    controller.initialize(argc,argv);

    return a.exec();
}
