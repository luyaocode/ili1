#ifndef CALC_INTERFACE_H
#define CALC_INTERFACE_H

// 函数指针类型（必须与动态库导出函数签名一致）
using CalcFunc = int (*)(int a, int b);          // 计算函数
using GetVersionFunc = const char* (*)();        // 版本函数

#endif // CALC_INTERFACE_H
