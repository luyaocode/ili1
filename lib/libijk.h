#ifndef LIBIJK__
#define LIBIJK__

#include <functional>
#include "common/lock_free_queue.h"

#define NAMESPACE_BEGIN(x) namespace x {
#define NAMESPACE_END(x) }

using UINT64 = unsigned long long;

NAMESPACE_BEGIN(IJK)
// 计算fib(93) -O3 40ns左右，极致优化可达 20 ns
UINT64 fibIter(UINT64 n);
// 计算fib(93) -O1 50ns左右，仅支持O0、O1
UINT64 fibIterAsm(UINT64 n);

// 从fib(0)渐进到fib(93) 共n百万次
// -O0 aver 300 ns
// -O1 aver 50-60 ns
// -Ofast -march=native -mtune=native -flto -fprofile-generate -fipa-cp-clone -floop-interchange -floop-parallelize-all -funroll-loops -finline-functions
// cpp aver 20 ns
// -O3 total 25ms 左右
double fibIterTest(float n = 1,bool print = true);

// 执行n百万次函数f,返回耗时ms
double callFunc(const std::function<int()>& f,float n = 1);
// 并行执行n百万次函数f,返回耗时ms
double callFuncP(const std::function<int()>& f,float n = 1);

NAMESPACE_END(IJK)

//extern "C"
//{
//    __attribute__((visibility("default")))
//}













#endif
