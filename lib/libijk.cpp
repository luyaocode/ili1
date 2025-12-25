#include "libijk.h"
#include <time.h>
#include <iostream>
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <omp.h>
#include <thread>
#include <chrono>

#define __PT__ printf("%s","===========================================================\n");
#define UNUSED(x) (void)x;

constexpr const int MILLION_TIMES = 1000000;
constexpr const char* COPYRIGHT ="😺😺😺😺😺IJK.lib😺😺😺😺😺\n";

using namespace std;

NAMESPACE_BEGIN(IJK)

constexpr const int FIB_MAX_64 = 93;

UINT64 fibIter(UINT64 n) {
    if (n < 1||n>93) return 0;
    UINT64 a = 0, b = 1;
    while (--n) {
        UINT64 t = a + b;
        a = b;
        b = t;
    }
    return b;
}

UINT64 fibIterAsm(UINT64 n)
{
        UINT64 result;
        UINT64  ta=0,tb=0,tc=0,tcnt=0;
        asm volatile(
                "cmpq $1,%[n]   \n"
                "jle .L_end     \n" // 小于1退出
                //
                "movq $0,%[a]   \n"
                "movq $1,%[b]   \n"
                "movq %[n],%[cnt]\n"
                "subq $1,%[cnt]  \n"

        ".L_loop:               "
                "movq %[a],%[c] \n"
                "addq %[b],%[c] \n"
                "movq %[b],%[a]  \n"
                "movq %[c],%[b]  \n"
                "subq $1,%[cnt] \n"
                "jnz .L_loop    \n"
        ".L_end:                "
                "movq %[b],%[res]\n"
                : [res] "=r" (result), // 输出，只写
                  [a] "+r" (ta),
                  [b] "+r" (tb),
                  [c] "+r" (tc),
                  [cnt] "+r" (tcnt) // 输出，读写
                : [n] "r" (n)   // 输入，只读
                : "cc","memory" // 破坏寄存器
        );
        return result;
}


double fibIterTest(float n /*=1*/,bool print /*=true*/)
{
        UINT64 t = MILLION_TIMES*n;
        if(t==0)
        {
            return 0;
        }
	volatile int flag=0;
	clock_t start = clock(); 
        for(volatile size_t i=0;i<t;++i){
                flag += fibIter((int)(((float)(i))/t*FIB_MAX_64));
	}
	clock_t end = clock();
	double duration_ms = ((double)(end-start)/CLOCKS_PER_SEC)*1000;
        if(print)
        {
            cout<<"[ijklib]: "<<__func__<<"\ttimes(mills): "<<n<<"\tduration(ms): "<<duration_ms<<endl;
        }
        return duration_ms;
}

double callFunc(const std::function<int()> &f, float n/*=1*/)
{
    UINT64 t = (UINT64)(MILLION_TIMES*n);
    if(t==0)
    {
        return 0;
    }
    volatile int flag=0; // 强制读内存，防止编译器优化
    clock_t start = clock();
    for(volatile size_t i=0;i<t;++i){
        flag+=f();
    }
    clock_t end = clock();
    double duration_ms = ((double)(end-start)/CLOCKS_PER_SEC)*1000;
    double average_ns =duration_ms*MILLION_TIMES/t;
    cout<<"[ijklib]: "<<__func__<<" times(mills): "<<n<<"\tduration(ms): "<<duration_ms<<"\taverage(ns):"<<average_ns<<endl;
    return duration_ms;
}
double callFuncP(const std::function<int ()> &f, float n/*=1*/)
{
    UINT64 t = (UINT64)(MILLION_TIMES*n);
    if(t==0)
    {
        return 0;
    }
    volatile int flag=0; // 强制读内存，防止编译器优化
    const int CPU_CORES = std::thread::hardware_concurrency()?:4;
    auto start = std::chrono::high_resolution_clock::now();
#pragma omp parallel num_threads(CPU_CORES)
    {
#pragma omp for
        for(volatile size_t i=0;i<t;++i){
            flag+=f();
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    // 计算程序真实流逝时间
    double duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    double average_ns =duration_ms*MILLION_TIMES/t;
    cout<<"[ijklib]: "<<__func__<<"\tCPU cores: "<<CPU_CORES<<"\ttimes(mills): "<<n<<"\tduration(ms): "<<duration_ms<<"\taverage(ns):"<<average_ns<<endl;
    return duration_ms;
}

NAMESPACE_END(IJK)

auto print_copyright = []()
{
    printf("%s",COPYRIGHT);
};

auto print_host_info = []()
{
    pid_t hostPid = getpid();
    char hostExePath[1024] ={0};
    char hostCmdline[1024] = {0};
    snprintf(hostExePath,sizeof(hostExePath),"/proc/%d/exe",hostPid);
    auto $_$ = readlink(hostExePath,hostExePath,sizeof(hostExePath)-1);
    UNUSED($_$)
    snprintf(hostCmdline,sizeof(hostCmdline),"/proc/%d/cmdline",hostPid);
    FILE* fCmdline = fopen(hostCmdline,"r");
    if(fCmdline)
    {
        $_$ = fread(hostCmdline,1,sizeof(hostCmdline)-1,fCmdline);
        UNUSED($_$)
        fclose(fCmdline);
        for(size_t i=0;i<strlen(hostCmdline);++i)
        {
            if(hostCmdline[i]=='\0')
            {
                hostCmdline[i] = ' ';
            }
        }
    }
    printf("host_pid: %d\n",hostPid);
    printf("host_exe_path: %s\n",hostExePath);
    printf("host_cmd_line: %s\n",hostCmdline);
};

__attribute__((constructor))
void onstart()
{
    __PT__
    print_copyright();
    print_host_info();
    __PT__
}




