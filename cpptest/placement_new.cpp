//[ijklib]: callFunc times(mills): 1	duration(ms): 202.893	average(ns):202.893
//testStack: 202.893
//[ijklib]: callFunc times(mills): 1	duration(ms): 3263.81	average(ns):3263.81
//testHeap: 3263.81

#include <iostream>
#include <new>
#include <cstring>
#include "libijk.h"
using namespace std;

constexpr const int N =10;
constexpr const int SIZE = 1024;
struct StA{int data=0;};
struct StL{char data[SIZE];};

int testStack()
{
    int result = 0;
    char buffer[sizeof(StA)*N];
    StA* p=new (buffer) StA;
    for(int i=0;i<N;++i)
    {
        p->data=i*789;
        p+=1;
    }
    for(int i=0;i<N;++i)
    {
        p-=1;
        result+=(p->data)%10;
    }
    return result;
}
int testHeap()
{
    int result = 0;
    StA* p = new StA[N];
    for(int i=0;i<N;++i)
    {
        p->data=i*789;
        p+=1;
    }
    for(int i=0;i<N;++i)
    {
        p-=1;
        result+=(p->data)%10;
    }
    return result;
}

int main(){
    cout<<"testStack: "<<IJK::callFunc(testStack)<<endl;
    cout<<"testHeap: "<<IJK::callFunc(testHeap)<<endl;

    char buffer[sizeof(StL)*N];
    StL* p1=new (buffer) StL;
    for(int i=0;i<N;++i)
    {
        memset(p1,i*11,sizeof(StL));
            p1+=1;
    }
    StL* p2[N];
    for(int i=0;i<N;++i)
    {
        p2[i]=new StL;
        memset(p2[i],i*11,sizeof(StL));
    }

    auto f1 = [p1]()->int
    {
        auto p = p1;
        int r1=0;
        for(int i=0;i<N;++i)
        {
            p-=1;
            for(int j=0;j<SIZE;++j)

            {r1+=p->data[j]%10;}
        }
        return r1;
    };

    auto f2 = [p2]()->int
    {
        auto p=p2;
        int r2=0;
        for(int i=0;i<N;++i)
        {
            for(int j=0;j<SIZE;++j)
            {r2+=p[i]->data[j]%10;}
        }
        return r2;
    };
    cout<<"f1 res: "<<f1()<<endl;
    cout<<"f2 res: "<<f2()<<endl;
    cout<<"f1: "<<IJK::callFunc(f1)<<endl;
    cout<<"f2: "<<IJK::callFunc(f2)<<endl;

    return 0;
}
