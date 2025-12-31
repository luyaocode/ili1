#include <QString>
#include <QtTest>
#include <QApplication>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include "unify/asynctaskrunner.hpp"
#include "unify/asynctaskmanager.hpp"
#include "unify/dynamic_library.hpp"
#include "calc_interface.h"
#include "MyWidget.h"
#include "ClassN.h"
#include "commontool/mousesimulator.h"

USING_NAMESAPCE(unify)

class UintTest : public QObject
{
    Q_OBJECT

public:
    UintTest();

private:
    void test_async_task();
    void test_async_task_mgr();
    void test_dynamic_library();
    void test_SignalDebugger();
    void test_TryLock();
private Q_SLOTS:
    void test_mouseSimulator();
};

UintTest::UintTest()
{
}

void sleep_ms(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
// Linux下复制文件（覆盖旧库）
void replace_library(const std::string &src, const std::string &dest)
{
    std::string cmd = "cp -f " + src + " " + dest;
    int         ret = system(cmd.c_str());
    if (ret != 0)
    {
        throw std::runtime_error("替换库失败: " + cmd);
    }
}

void UintTest::test_async_task()
{
    std::cout << "=====================================" << std::endl;
    std::cout << "开始执行所有AsyncTaskRunner测试用例（C++11）" << std::endl;
    std::cout << "=====================================" << std::endl;

    // ============== 测试1：基础任务（无返回值，百分比进度）==============
    std::cout << "\n=== 测试1：基础无返回值任务 ===" << std::endl;
    {
        bool  callback_called = false;
        float last_progress   = 0.0f;

        // C++11：显式声明lambda参数类型，不能用auto
        SimpleAsyncTask<> task(
            // TaskFunc的参数类型：两个std::function
            [](const std::function<void(float, float, std::string)> &update_progress,
               const std::function<bool()>                          &is_cancelled) {
                for (int i = 1; i <= 3; ++i)
                {
                    if (is_cancelled())
                        break;
                    float pct = (i / 3.0f) * 100;
                    update_progress(pct, pct, "Step " + std::to_string(i));
                    sleep_ms(100);
                }
            },
            nullptr,
            // ProgressCallback类型：const TaskProgress<float>&
            [&](const TaskProgress<float> &progress) {
                callback_called = true;
                last_progress   = progress.percentage;
                std::cout << "进度回调：" << progress.percentage << "%，消息：" << progress.message << std::endl;
            });

        task.start();
        sleep_ms(400);

        assert(task.get_state() == EM_TASK_STATE::COMPLETED);
        assert(callback_called);
        assert(last_progress == 100.0f);
        std::cout << "测试1通过！" << std::endl;
    }

    // ============== 测试2：带返回值的任务 ==============
    std::cout << "\n=== 测试2：带返回值的任务 ===" << std::endl;
    {
        AsyncTaskRunner<int> task(
            [](const std::function<void(float, float, std::string)> &update_progress,
               const std::function<bool()>                          &is_cancelled) {
                int sum = 0;
                for (int i = 1; i <= 100; ++i)
                {
                    if (is_cancelled())
                        throw std::runtime_error("任务被取消");
                    sum += i;
                    if (i % 10 == 0)
                    {
                        float pct = (i / 100.0f) * 100;
                        update_progress(pct, i, "已计算到 " + std::to_string(i));
                        sleep_ms(200);
                    }
                }
                return sum;
            },
            nullptr,
            [](const TaskProgress<float> &progress) {
                std::cout << "进度回调：" << progress.percentage << "%，当前值：" << progress.custom_data << std::endl;
            });

        task.start();
        int result = task.get_result();

        assert(task.get_state() == EM_TASK_STATE::COMPLETED);
        assert(result == 5050);
        std::cout << "任务返回结果：" << result << std::endl;
        std::cout << "测试2通过！" << std::endl;
    }

    // ============== 测试3：任务取消 ==============
    std::cout << "\n=== 测试3：任务取消 ===" << std::endl;
    {
        SimpleAsyncTask<> task(
            [](const std::function<void(float, float, std::string)> &update_progress,
               const std::function<bool()>                          &is_cancelled) {
                for (int i = 1; i <= 10; ++i)
                {
                    if (is_cancelled())
                    {
                        std::cout << "任务检测到取消请求，退出执行" << std::endl;
                        return;
                    }
                    update_progress(i * 10, i * 10, "运行中...");
                    sleep_ms(100);
                }
            },
            nullptr,
            [](const TaskProgress<float> &progress) {
                std::cout << "进度回调：" << progress.percentage << "%" << std::endl;
            });

        task.start();
        sleep_ms(350);
        task.cancel();
        sleep_ms(200);

        assert(task.get_state() == EM_TASK_STATE::CANCELLED);
        bool exception_thrown = false;
        try
        {
            task.get_result();
        }
        catch (const std::runtime_error &e)
        {
            exception_thrown = true;
            std::cout << "捕获预期异常：" << e.what() << std::endl;
        }
        assert(exception_thrown);
        std::cout << "测试3通过！" << std::endl;
    }

    // ============== 测试4：任务抛出异常 ==============
    std::cout << "\n=== 测试4：任务抛出异常 ===" << std::endl;
    {
        SimpleAsyncTask<> task(
            [](const std::function<void(float, float, std::string)> &update_progress,
               const std::function<bool()>                          &is_cancelled) {
                Q_UNUSED(is_cancelled)
                update_progress(30, 30, "执行中...");
                sleep_ms(100);
                throw std::invalid_argument("无效的参数：测试异常抛出");
            },
            nullptr,
            [](const TaskProgress<float> &progress) {
                std::cout << "进度回调：" << progress.percentage << "%" << std::endl;
            });

        task.start();
        sleep_ms(200);

        assert(task.get_state() == EM_TASK_STATE::FAILED);
        assert(task.get_error() == "无效的参数：测试异常抛出");
        std::cout << "任务错误信息：" << task.get_error() << std::endl;

        bool exception_thrown = false;
        try
        {
            task.get_result();
        }
        catch (const std::invalid_argument &e)
        {
            exception_thrown = true;
            std::cout << "捕获预期异常：" << e.what() << std::endl;
        }
        assert(exception_thrown);
        std::cout << "测试4通过！" << std::endl;
    }

    // ============== 测试5：动态修改进度回调 ==============
    std::cout << "\n=== 测试5：动态修改进度回调 ===" << std::endl;
    {
        std::string last_msg_v1;
        std::string last_msg_v2;

        SimpleAsyncTask<> task([](const std::function<void(float, float, std::string)> &update_progress,
                                  const std::function<bool()>                          &is_cancelled) {
            for (int i = 1; i <= 5; ++i)
            {
                if (is_cancelled())
                    break;
                update_progress(i * 20, i * 20, "版本" + std::to_string(i));
                sleep_ms(150);
            }
        });

        task.start();
        sleep_ms(200);

        // 动态设置第1个回调
        task.set_progress_callback([&](const TaskProgress<float> &progress) {
            std::cout << "[回调V1] 进度：" << progress.percentage << "%，消息：" << progress.message << std::endl;
            last_msg_v1 = progress.message;
        });

        sleep_ms(300);

        // 动态切换第2个回调
        task.set_progress_callback([&](const TaskProgress<float> &progress) {
            std::cout << "[回调V2] 进度：" << progress.percentage << "%，消息：" << progress.message << std::endl;
            last_msg_v2 = progress.message;
        });

        sleep_ms(300);

        assert(!last_msg_v1.empty());
        assert(!last_msg_v2.empty());
        std::cout << "V1回调最后消息：" << last_msg_v1 << std::endl;
        std::cout << "V2回调最后消息：" << last_msg_v2 << std::endl;
        assert(task.get_state() == EM_TASK_STATE::COMPLETED);
        std::cout << "测试5通过！" << std::endl;
    }

    // ============== 测试6：自定义进度数据类型 ==============
    std::cout << "\n=== 测试6：自定义进度数据 ===" << std::endl;
    {
        struct CustomProgress
        {
            int current_step;
            int remaining_ms;
        };

        AsyncTaskRunner<std::string, CustomProgress> task(
            [](const std::function<void(float, CustomProgress, std::string)> &update_progress,
               const std::function<bool()>                                   &is_cancelled) {
                const int total_steps = 4;
                const int step_ms     = 100;
                for (int i = 1; i <= total_steps; ++i)
                {
                    if (is_cancelled())
                        break;
                    float          pct = (i / (float)total_steps) * 100;
                    CustomProgress data {i, (total_steps - i) * step_ms};
                    update_progress(pct, data, "处理中");
                    sleep_ms(step_ms);
                }
                return std::string("自定义进度任务完成");
            },
            nullptr,
            [](const TaskProgress<CustomProgress> &progress) {
                std::cout << "进度：" << progress.percentage << "%，"
                          << "当前步骤：" << progress.custom_data.current_step << "，"
                          << "剩余时间：" << progress.custom_data.remaining_ms << "ms" << std::endl;
            });

        task.start();
        std::string result = task.get_result();

        assert((task.get_state() == EM_TASK_STATE::COMPLETED));
        assert(result == "自定义进度任务完成");
        std::cout << "任务返回：" << result << std::endl;
        std::cout << "测试6通过！" << std::endl;
    }

    std::cout << "\n=====================================" << std::endl;
}

void UintTest::test_async_task_mgr()
{
    std::cout << "test_async_task_mgr start" << std::endl;
    auto &task_mgr = unify::AsyncTaskManager::get_instance();

    // 设置最大并发数为3
    task_mgr.set_max_concurrent_tasks(3);

    // 添加多个任务
    for (int i = 0; i < 10; ++i)
    {
        auto task_id = task_mgr.add_task<int>(
            [i](const std::function<void(float, float, std::string)> &update_progress,
                const std::function<bool()>                          &is_cancelled) -> float {
                for (int step = 0; step <= 100; step += 10)
                {
                    if (is_cancelled())
                        break;
                    update_progress(step, i, "Task " + std::to_string(i) + " step " + std::to_string(step));
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
                std::cout << "Task " << i << " completed" << std::endl;
                return 1;
            },
            nullptr,
            [i](const unify::TaskProgress<float> &progress) {
                std::cout << "Task " << i << " progress: " << progress.percentage
                          << "% (custom: " << progress.custom_data << ")" << std::endl;
            },
            [](EM_TASK_STATE state, int result) {
                Q_UNUSED(state)
                std::cout << "result: " << result << std::endl;
            });

        std::cout << "Added task " << i << " with ID: " << task_id << std::endl;
    }

    // 等待所有任务完成
    while (task_mgr.get_running_task_count() > 0)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Running tasks: " << task_mgr.get_running_task_count() << std::endl;
    }

    std::cout << "All tasks completed" << std::endl;
}

void UintTest::test_dynamic_library()
{
    try
    {
        // 动态库路径定义
        const std::string lib_path = "/home/chenluyao/target_dir/demo/libcalc.so";
        const std::string lib_v1   = "/home/chenluyao/target_dir/demo/libcalc_v1.so";
        const std::string lib_v2   = "/home/chenluyao/target_dir/demo/libcalc_v2.so";

        // ========== 1. 初始加载版本1 ==========
        std::cout << "=== 加载版本1动态库 ===" << std::endl;
        // 先将v1复制为默认库
        replace_library(lib_v1, lib_path);

        // 加载库并获取函数
        DynamicLibrary lib(lib_path);
        CalcFunc       calc        = lib.get_function<CalcFunc>("calc");
        GetVersionFunc get_version = lib.get_function<GetVersionFunc>("get_version");

        // 测试功能
        std::cout << "版本信息: " << get_version() << std::endl;
        std::cout << "calc(3,5) = " << calc(3, 5) << std::endl;  // 预期：8（加法）

        // ========== 2. 运行时热替换版本2 ==========
        std::cout << "\n=== 开始热替换动态库 ===" << std::endl;
        // 卸载旧库（也可直接调用lib.load，内部会自动卸载）
        lib.unload();
        // 替换磁盘文件（v2覆盖默认库）
        replace_library(lib_v2, lib_path);
        // 重新加载新库
        lib.load(lib_path);

        // 更新函数指针
        calc        = lib.get_function<CalcFunc>("calc");
        get_version = lib.get_function<GetVersionFunc>("get_version");

        // 测试新版本功能
        std::cout << "\n=== 替换后版本2 ===" << std::endl;
        std::cout << "版本信息: " << get_version() << std::endl;
        std::cout << "calc(3,5) = " << calc(3, 5) << std::endl;  // 预期：15（乘法）
    }
    catch (const std::exception &e)
    {
        std::cerr << "错误: " << e.what() << std::endl;
        return;
    }
}

void test_SignalDebugger()
{
}

void UintTest::test_SignalDebugger()
{
    // 1. 规范初始化 QApplication
    int          argc   = 0;
    char        *argv[] = {nullptr};
    QApplication app(argc, argv);

    // 2. 堆分配对象，确保生命周期可控
    SignalEmitter *emitter  = new SignalEmitter();
    MyWidget      *mywidget = new MyWidget();

    // 3. 断开 mywidget 所有已有连接（清除 QtTest/自动连接的干扰）
    mywidget->disconnect();

    // 4. 仅连接自定义信号-槽（精准绑定，无多余连接）
    QMetaObject::Connection conn =
        connect(emitter, &SignalEmitter::sigClickBtn, mywidget, &MyWidget::onCustomBtnClicked,  // 绑定新槽函数名
                Qt::DirectConnection  // 强制同步执行，无队列延迟
        );
    Q_ASSERT(conn);  // 验证连接成功

    // 5. 手动发射自定义信号（确保仅触发一次）
    qDebug() << "手动发射 sigClickBtn 信号";
    emitter->clickBtn();

    // 6. 立即断开连接（防止后续析构信号触发槽函数）
    disconnect(conn);

    // 7. 验证槽函数仅被 sigClickBtn 触发（无后续干扰）
    QTimer::singleShot(0, &app, [&]() {
        // 退出事件循环前，手动释放对象（此时无连接，destroyed 信号不会触发槽）
        delete mywidget;
        delete emitter;
        app.quit();  // 退出事件循环
    });

    // 8. 启动事件循环（仅处理必要逻辑，立即退出）
    app.exec();
}

void UintTest::test_TryLock()
{
    ClassN n;
    n.tryLock();
}

// 鼠标画圆形轨迹（以屏幕中心(960,540)为圆心）
void drawMouseCircle(MouseSimulator * mouseSim) {
    // 配置参数
    const int centerX = 960;    // 圆心X坐标（屏幕中心）
    const int centerY = 540;    // 圆心Y坐标（屏幕中心）
    const int radius = 200;     // 圆的半径（可根据需要调整）
    const int steps = 1000;      // 绘制圆的步数（步数越多，圆越平滑）
    const int delayMs = 10;     // 每步移动的延时（毫秒，越小速度越快）

    // 遍历0~360度，计算每个点的坐标并移动鼠标
    for (int i = 0; i <= steps; ++i) {
        // 将角度转换为弧度（三角函数需要弧度值）
        double angle = 2 * M_PI * i / steps;

        // 圆的参数方程计算当前点坐标
        int x = centerX + radius * cos(angle);
        int y = centerY + radius * sin(angle);

        // 移动鼠标到当前点
        mouseSim->moveMouse(x, y);

        // 短暂延时，让轨迹可见（使用ms级延时更细腻）
        QThread::msleep(delayMs);
    }
}

void UintTest::test_mouseSimulator()
{
    // 1. 规范初始化 QApplication
    int          argc   = 0;
    char        *argv[] = {nullptr};
    QApplication app(argc, argv);
    // 获取鼠标模拟器实例
    MouseSimulator *mouseSim = MouseSimulator::getInstance();

    // ========== 接口使用示例 ==========
    int curX, curY;
    // 1. 获取当前鼠标位置
    if (mouseSim->getCurrentMousePos(curX, curY))
    {
        qInfo() << "当前鼠标位置：" << curX << "," << curY;
    }

    // 2. 移动鼠标到屏幕中心（假设屏幕分辨率1920x1080）
//    mouseSim->moveMouse(960, 540);
//    QThread::sleep(1);  // 等待1秒，便于观察
    drawMouseCircle(mouseSim);

//    // 3. 左键单击屏幕中心
//    mouseSim->clickMouse(MouseSimulator::LeftButton, 960, 540);
//    QThread::sleep(1);

//    // 4. 右键单击当前位置（不指定坐标）
//    mouseSim->clickMouse(MouseSimulator::RightButton);
//    QThread::sleep(1);

//    // 5. 中键双击
//    mouseSim->doubleClickMouse(MouseSimulator::MiddleButton, 960, 540);
//    QThread::sleep(1);

//    // 6. 鼠标相对移动（右移50px，下移50px）
//    mouseSim->moveMouseRelative(50, 50);
//    QThread::sleep(1);

//    // 7. 鼠标滚轮向下滚动2步
//    mouseSim->scrollWheel(MouseSimulator::WheelDown, 2);
//    QThread::sleep(1);

    // 8. 鼠标左键按下并释放（模拟拖拽）
//    mouseSim->pressMouse(MouseSimulator::LeftButton);
//    QThread::msleep(500);
//    mouseSim->moveMouseRelative(100, 0);  // 右拖100px
//    mouseSim->releaseMouse(MouseSimulator::LeftButton);

    qInfo() << "所有鼠标操作执行完成";
//    app.exec();
}

QTEST_APPLESS_MAIN(UintTest)

#include "tst_uinttest.moc"
