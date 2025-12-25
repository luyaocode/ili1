#include "b.h"
#include <cstdint>
#include <stack>
#include <queue>
#include <math.h>
#include <algorithm>

using namespace std;

void aStar(const State& s, PointSet &path)
{
    UNUSED(s)
    UNUSED(path)
//    long long  score_need = s.score_need;
//    long long player_expr = s.player_expr;
//    Matrix* pos = s.pos;
//    Matrix* flag = s.flag;
//    PointSet* wall = s.wall;
//    Point player = s.player;
//    Point door = s.door;
}

long long calExpr(const State &s, const Point& p, long long exp)
{

    auto r = p.x;
    auto c = p.y;
    auto& flag = *(s.flag);
    auto& pos = *(s.pos);
    if(s.player == p)
    {
        return exp;
    }
    if(s.door == p)
    {
        return exp-s.score_need;
    }
    auto res = exp;
    switch(flag[r][c])
    {
    case INT_MULTIPLY:
        res*=abs(pos[r][c]);
        break;
    case INT_DEVISION:
    {
        if(pos[r][c]!=0)
        {
           res/=abs(pos[r][c]);
        }
        break;
    }
    default:
        res+=pos[r][c];
        break;
    }
    return res;
}

State::State()
{

}

State::State(long long sneed, long long exp, Matrix *ps, Matrix *flg, PointSet *wal, const Point &pl, const Point &dor):
    score_need(sneed),
    player_expr(exp),
    pos(ps),
    flag(flg),
    wall(wal),
    player(pl),
    door(dor)
{

}
