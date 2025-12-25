#include <iostream>
#include "libijk.h"
#include <time.h>

using namespace std;
using namespace Fib;

int clc()
{
    return Fib::fibIter(93);
}

template<typename T>
class CBaseT
{
public:
        int calc()
	{
                return static_cast<T*>(this)->calc();
	}
};

class CDrivedTa:public CBaseT<CDrivedTa>
{
public:
        int calc()
	{
            return clc();
	}
};

class CBase
{
public:
        virtual int calc()
        {
            return 0;
        }
};

class CDrived : public CBase
{
public:
    int calc() override
    {
        return clc();
    }
};

int main()
{
#ifdef CRTP
	CDrivedTa d;
	auto* p =&d;
#else
    CDrived d;
    auto* p =&d;
#endif
    auto func = [p]()->int{
        return p->calc();
    };
    volatile double sum =0;
    int t = 10;
    for(int i=0;i<t;++i)
    {
        sum+=IJK::callFunc(func);
    }
    cout<<"aver: "<<sum/t<<endl;
    return 0;
}
