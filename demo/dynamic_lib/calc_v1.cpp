#include "calc_interface.h"
#include <string>

extern "C"
{
// Linux下无需显式导出（-fPIC编译即可）
int calc(int a, int b) {
    return a + b; // 版本1：加法
}

const char* get_version() {
    return "calc_v1.0 (加法功能)";
}
}

