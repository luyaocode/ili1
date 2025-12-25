#ifndef D_H__
#define D_H__

#include <vector>
#include <utility>
#include <math.h>

using std::vector;
using std::pair;

// 全局变量：网格大小、人物初始位置（行row，列col）
#define GRID_SIZE 9
#define NUM_SIZE 7
//↑ ↓ ← →
//←↓→ 🐭
constexpr const char* POS_PLAYER    = "   😺   ";
constexpr const char* POS_DOOR      = "   🐭   ";
constexpr const char* POS_WALL      = "   墙   ";
constexpr const char* ARROW_LEFT    = "   .   ";
constexpr const char* ARROW_UP      = "   .   ";
constexpr const char* ARROW_RIGHT   = "   .   ";
constexpr const char* ARROW_DOWN    = "   .   ";
// 评级
constexpr const char* TIER_NOOB     = "逊";
constexpr const char* TIER_WEAK     = "弱";
constexpr const char* TIER_SOSO     = "行";
constexpr const char* TIER_SOLID    = "强";
constexpr const char* TIER_EPIC     = "绝";
constexpr const char* TIER_GOD      = "神";
// 方格标志位
constexpr const int INT_NOTHING     = 0;                // 无状态
constexpr const int INT_ARROW_LEFT  = -1;               // 左箭头
constexpr const int INT_ARROW_UP    = -2;               // 上箭头
constexpr const int INT_ARROW_RIGHT = -3;               // 右箭头
constexpr const int INT_ARROW_DOWN  = -4;               // 下箭头
constexpr const int INT_MULTIPLY    = -5;               // 乘法
constexpr const int INT_DEVISION    = -6;               // 除法
constexpr const int INT_WALL        = -0xFF;            // 墙体
// 按键
constexpr const int INT_KEY_L    = 'L'; // 76
constexpr const int INT_KEY_R    = 'R'; // 82
constexpr const int INT_KEY_U    = 'U'; // 85
constexpr const int INT_KEY_D    = 'D'; // 68
// 基本常量
constexpr const int LEVEL_DESIGN = 100;                 // 设计等级
constexpr const int LEVEL_DESIGN_MAX = LEVEL_DESIGN*100;               // 设计最高等级
constexpr const int STEP_REMAINING = GRID_SIZE*GRID_SIZE;                 // 初始剩余步数
constexpr const int STEP_STEP_ADD = GRID_SIZE*2; // 通关奖励步数
constexpr const int MIN_SCORE = -1000;                  // 最小分数
constexpr const int MAX_SCORE =  1000;                  // 最大分数
constexpr const int DESC_TIME  = 60.0;                  // 倒计时预设时间
constexpr const int HARD_LEVEL_POW =  100;             // 指数难度等级
constexpr const int HARD_LEVEL_MAX =  1000;             // 难度等级
constexpr const int DISTANCE_MAX =  2*pow(GRID_SIZE-1,2);   // 最大欧式距离
constexpr const int DISTANCE_MAX_MIN =  2*pow(GRID_SIZE/2,2);   // 高难度最大最小欧式距离，player位于中心
constexpr const int DISTANCE_MIN =  2*pow(1,2);             // 最小欧式距离
constexpr const int MODE_BASE =  10;                    // 基本模数
constexpr const int DIST_NOT_ACCESS   =  -1;            // 不可达距离

const vector<pair<int,int>> DIRECTIONS={{-1,0},{1,0},{0,-1},{0,1}}; // 左右上下

// 点结构
struct Point
{
    int x =0;
    int y =0;
    Point* prev = nullptr;
    Point():x(0),y(0){}
    Point(int a,int b):x(a),y(b),prev(nullptr){}
    Point(const Point& p):x(p.x),y(p.y),prev(p.prev){}
    ~Point(){
        if(prev)
        {
            delete prev;
            prev = nullptr;
        }
    }
    // 浅拷贝
    Point& operator=(const Point& p)
    {
        if(this==&p)
        {
            return *this;
        }
        x=p.x;
        y=p.y;
        prev = p.prev;
        return *this;
    }
    // 是否数值相同
    bool operator == (const Point& p) const
    {
        return x==p.x&&y==p.y;
    }
    bool operator != (const Point& p) const
    {
        return x!=p.x||y!=p.y;
    }
    // 小于
    bool operator <(const Point& p) const
    {
        return x+y<p.x+p.y;
    }
    // 大于
    bool operator >(const Point& p) const
    {
        return x+y>p.x+p.y;
    }
    // 点加法
    Point operator+(const pair<int,int>& pair) const
    {
        return {this->x+pair.first,this->y+pair.second};
    }
    // 点坐标是否有效
    bool isValid() const
    {
        return (x>=0)&&(x<GRID_SIZE)&&(y>=0)&&(y<GRID_SIZE);
    }
};
using Path = vector<Point>;



#ifndef UNUSED
#define UNUSED(x) (void)x;
#endif






#endif
