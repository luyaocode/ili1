#include "calc_interface.h"
#include <string>

extern "C"
{
int calc(int a, int b) {
    return a * b; // 版本2：乘法
}

const char* get_version() {
    return "calc_v2.0 (乘法功能)";
}
}

