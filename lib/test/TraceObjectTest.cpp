//
// COMPILE: g++ -std=c++11 -O3 -Icommon test/TraceObjectTest.cpp -o test/TraceObjectTest

#include "../common/TraceObject.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <unistd.h>

// 测试辅助函数
void testPassed(const std::string& testName) {
    std::cout << "\033[32m[PASSED] " << testName << "\033[0m" << std::endl;
}

void testFailed(const std::string& testName, const std::string& reason) {
    std::cout << "\033[31m[FAILED] " << testName << ": " << reason << "\033[0m" << std::endl;
}

void testObjectLifetime() {
    const std::string testName = "Object lifetime calculation";
    const int sleepMs = 500;

    auto start = std::chrono::high_resolution_clock::now();
    {
        TRACEOBJECT6("LifetimeTest")
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    }
    auto end = std::chrono::high_resolution_clock::now();

    // 验证对象存在时间是否在合理范围内
    auto actualDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (actualDuration >= sleepMs && actualDuration <= sleepMs + 10) {  // 允许10ms误差
        testPassed(testName);
    } else {
        testFailed(testName, "Lifetime incorrect: " + std::to_string(actualDuration) + "ms");
    }
}

int main() {
    std::cout << "=== Running TraceObject Tests ===" << std::endl;
    ORANGELOG("ttttdfd")
    std::thread tt(testObjectLifetime);
    usleep(1000000);
    std::thread t(testObjectLifetime);
    t.join();
    tt.join();
    return 0;
}
