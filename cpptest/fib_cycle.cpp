#include <iostream>
#include <cstdlib>
#include <chrono>
#include <time.h>
using namespace std;
using UINT64=unsigned long long;
UINT64 fibIter(UINT64 n)
{
	if(n==1)
	{
		return 0;
	}
	if(n==2)
	{
		return 1;
	}
	UINT64 result;
	UINT64 a=0,b=1;
	for(int i=0;i<n-1;i++)
	{
		result=a+b;
		a=b;
		b=result;
	}
	return result;
}

const int MILLION_TIMES = 1000000;
int main(int argc,char* argv[])
{
	int t = 93;
	int u = 1*MILLION_TIMES;
	UINT64 result;
	volatile int i=0;
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
	volatile UINT64 dummy;	
   	clock_t start = clock();
	for(;i<u;++i)
	{
		dummy+=fibIter(int((float(i)/u)*40)+53);
	}
    clock_t end = clock();

    double duration_s = (double)(end - start) / CLOCKS_PER_SEC;
    double duration_ns = duration_s * 1e9 / u;

	//auto endTime = chrono::steady_clock::now();
	//double duration_ms = chrono::duration_cast<chrono::milliseconds>(endTime-startTime).count();
	//double duration_ns = duration_ms*100000/u;
	cout<<"number: "<<t<<endl<<"times: "<<u<<endl<<"result: "<<result<<endl;
	cout<<"result: "<<result<<endl;
	cout<<"duration(s): "<<duration_s<<endl;
	cout<<"duration(ns): "<<duration_ns<<endl;
	return 0;
}
