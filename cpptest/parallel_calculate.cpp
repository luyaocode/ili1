//[ijklib]: callFunc times(mills): 1	duration(ms): 3816.39	average(ns):3816.39
//3816.39
//[ijklib]: callFuncP	CPU cores: 16	times(mills): 1	duration(ms): 368	average(ns):368
//368
// omp对于耗时较长的任务有明显提升
#include <iostream>
#include "libijk.h"

constexpr const double DURATION_FACTOR = 0.0001;
constexpr const double TASK_FACTOR = 1;
auto func = []()->int
{
    return Fib::fibIterTest(DURATION_FACTOR,false);
};

using namespace std;
int main()
{
    cout<<IJK::callFunc(func,TASK_FACTOR)<<endl;
    cout<<IJK::callFuncP(func,TASK_FACTOR)<<endl;
    return 0;
}
