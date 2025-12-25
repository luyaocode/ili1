#include <iostream>
#include <cstdlib>
#include <chrono>
#include <time.h>
using namespace std;
using UINT64=unsigned long long;
UINT64 fibIter(UINT64 n)
{
	UINT64 result;
	UINT64  ta=0,tb=0,tc=0,tcnt=0;
	asm volatile(
		"cmpq $1,%[n] 	\n"
		"jle .L_end 	\n" // 小于1退出
		// 
		"movq $0,%[a]	\n"
		"movq $1,%[b]	\n"
		"movq %[n],%[cnt]\n"
		"subq $1,%[cnt]  \n"

	".L_loop:		"
		"movq %[a],%[c]	\n"
		"addq %[b],%[c]	\n"
		"movq %[b],%[a]  \n"
		"movq %[c],%[b]  \n"
		"subq $1,%[cnt]	\n"
		"jnz .L_loop	\n"
	".L_end:		"
		"movq %[b],%[res]\n"
		: [res] "=r" (result), // 输出，只写
		  [a] "+r" (ta), 
		  [b] "+r" (tb),
		  [c] "+r" (tc),
		  [cnt] "+r" (tcnt) // 输出，读写
	   	: [n] "r" (n) 	// 输入，只读
		: "cc","memory" // 破坏寄存器
	);
	return result;
}

const int MILLION_TIMES = 1000000;
int main(int argc,char* argv[])
{
	int t = 93;
	int u = 1*MILLION_TIMES;
	UINT64 result;
	volatile int flag = 0;
	if(argc==2)
	{
		t=atoi(argv[1]);
	}
	if(argc==3)
	{
		t=atoi(argv[1]);
		u=atoi(argv[2])*MILLION_TIMES;
	}
	        result=fibIter(t);
//      auto startTime = chrono::steady_clock::now();

    clock_t start = clock();
    __sync_synchronize();
        for(int i=0;i<u;++i)
        {
                flag=fibIter(int((float(i)/u)*40)+53)%10;
        }
    __sync_synchronize();
    clock_t end = clock();

    double duration_s = (double)(end - start) / CLOCKS_PER_SEC;
    double duration_ns = duration_s * 1e9 / u;

        //auto endTime = chrono::steady_clock::now();
        //double duration_ms = chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count();
        //double duration_ns = duration_ms*100000/u;
        cout<<"number: "<<t<<endl<<"times: "<<u<<endl<<"result: "<<result<<endl;
        cout<<"result: "<<result<<endl;
        cout<<"flag: "<<flag<<endl;
        cout<<"duration(s): "<<duration_s<<endl;
        cout<<"duration(ns): "<<duration_ns<<endl;
        return 0;
}
