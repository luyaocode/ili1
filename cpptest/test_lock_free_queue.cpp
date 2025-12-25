#include "lib/libijk.h"
#include <thread>
#include <iostream>
#include <mutex>
#include <queue>
#include <cstring>
#include <chrono>
#include <cmath>

using namespace std;
using namespace IJK;

mutex g_mtx;
int g_egg=0;

CLockFreeQueue<int,(size_t)pow(2,22)> g_fq;
queue<int> g_q;

auto pushq_1 = []()->int
{
        lock_guard<mutex> lock(g_mtx);
        g_q.push(1);
        return 1;
};

auto pushq_2 = []()->int
{
        lock_guard<mutex> lock(g_mtx);
        g_q.push(2);
        return 1;
};

auto pushfq_1 = []()->int
{
        g_fq.enqueue(1);
        return 1;
};

auto pushfq_2 = []()->int
{
        g_fq.enqueue(2);
        return 1;
};

auto popq_1 = []()->int
{
        lock_guard<mutex> lock(g_mtx);
        g_q.pop();
        return 1;
};

auto popq_2 = []()->int
{
        lock_guard<mutex> lock(g_mtx);
        g_q.pop();
        return 1;
};

auto popfq_1 = []()->int
{
        int val;
        g_fq.dequeue(val);
        return 1;
};

auto popfq_2 = []()->int
{
        int val;
        g_fq.dequeue(val);
        return 1;
};

void testpushq()
{
    cout<<"test stl queue push:"<<endl;
    thread t1([&](){cout<<"t1:"<<IJK::callFunc(pushq_1)<<endl;});
    thread t2([&](){cout<<"t2:"<<IJK::callFunc(pushq_2)<<endl;});
    t1.join();
    t2.join();
    cout<<"size: "<<g_q.size()<<endl;
    int val;
    while(!g_q.empty())
    {
        val = g_q.front();
        g_egg+=val;
        g_q.pop();
    }
}

void testpushfq()
{
    cout<<"test lockfree queue push:"<<endl;
    thread t1([&](){cout<<"t1:"<<IJK::callFunc(pushfq_1)<<endl;});
    thread t2([&](){cout<<"t2:"<<IJK::callFunc(pushfq_2)<<endl;});
    t1.join();
    t2.join();
    cout<<"fq size: "<<g_fq.size()<<endl;
    int val;
    while(g_fq.dequeue(val))
    {
        g_egg+=val;
    }
    cout<<"fq size: "<<g_fq.size()<<endl;
}

void testpopq()
{
    cout<<"test stl queue pop:"<<endl;
    int time = 3000000;
    while(time-->0)
    {
        pushq_1();
    }
    cout<<"q size: "<<g_q.size()<<endl;
    thread t1([&](){cout<<"t1:"<<IJK::callFunc(popq_1,1)<<endl;});
    thread t2([&](){cout<<"t2:"<<IJK::callFunc(popq_2,1)<<endl;});
    t1.join();
    t2.join();
    cout<<"q size: "<<g_q.size()<<endl;
}

void testpopfq()
{
    cout<<"test lockfree queue pop:"<<endl;
    int time = 3000000;
    while(time-->0)
    {
        pushfq_1();
    }
    cout<<"fq size: "<<g_fq.size()<<endl;
    thread t1([&](){ try{cout<<"t1:"<<IJK::callFunc(popfq_1,1)<<endl;}catch(...){cout<<"t1 exception"<<endl;}});
    thread t2([&](){ try{cout<<"t1:"<<IJK::callFunc(popfq_2,1)<<endl;}catch(...){cout<<"t2 exception"<<endl;}});
    t1.join();
    t2.join();
    cout<<"fq size: "<<g_fq.size()<<endl;
}

int main(int argc,char** argv)
{
    int tt = -1;
    if(argc>1)
    {
        tt=stoi(argv[1]);
    }
    switch(tt)
    {
    case 1:
        testpushq();
        break;
    case 2:
        testpushfq();
        break;
    case 3:
        testpopq();
        break;
    case 4:
        testpopfq();
        break;
    default:
        cout<<"Usage: ./test_lock_free_queue [0-test queue,1-test lockfreequeue]"<<endl;
        break;
    }
    cout<<"g_egg:"<<g_egg<<endl;
    return 0;
}

