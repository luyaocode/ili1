//tsAt res: 394105500
//tsPtr res: 394105500
//[ijklib]: callFunc times(mills): 1	duration(ms): 723.957	average(ns):723.957
//tsAt: 723.957
//[ijklib]: callFunc times(mills): 1	duration(ms): 180.033	average(ns):180.033
//tsPtr: 180.033
#include <iostream>
#include <vector>
#include "libijk.h"
using namespace std;
constexpr const int N =1000;
struct StInt{int data;};

int main()
{
    vector<int> nums;
    for(int i=0;i<N;++i)
    {
        nums.push_back(i*789);
    }
    auto tsAt = [&]()->int
    {
        int result = 0;
        for(int i=0;i<N;++i)
        {
            result+=nums.at(i);
        }
        return result;
    };
    auto tsPtr = [&]()->int
    {
        int result = 0;
        for(int i=0;i<N;++i)
        {
            result+=nums[i];
        }
        return result;
    };
    cout<<"tsAt res: "<<tsAt()<<endl;
    cout<<"tsPtr res: "<<tsPtr()<<endl;
    cout<<"tsAt: "<<IJK::callFunc(tsAt)<<endl;
    cout<<"tsPtr: "<<IJK::callFunc(tsPtr)<<endl;

    return 0;
}
