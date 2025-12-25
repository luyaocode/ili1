// new: 开辟内存空间->调用构造函数初始化内存->返回内存首地址，重载new无法实现初始化其他对象
#include <iostream>
#include <cstdlib>
using namespace std;
class A{public:void* operator new(size_t size);void operator delete(void* p){free(p);}void show(){cout<<m_cData<<endl;}public:const char m_cData='A';};
class B{public:void* operator new(size_t size);void operator delete(void* p){free(p);}void show(){cout<<m_cData<<endl;}public:const char m_cData='B';};
void* A::operator new(size_t size){return malloc(sizeof(B));}
void* B::operator new(size_t size){return malloc(sizeof(A));}
int main(){A* a =new A;B* b=new B;cout<<a->m_cData<<endl;cout<<b->m_cData<<endl;return 0;}
