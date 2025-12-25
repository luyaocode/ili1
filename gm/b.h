#ifndef B_H__
#define B_H__

#include "d.h"
using Matrix = vector<vector<int>>;
using PointSet = vector<Point>;
struct State
{
    long long  score_need;
    long long player_expr;
    Matrix* pos;
    Matrix* flag;
    PointSet* wall;
    Point player;
    Point door;
    State();
    State(long long sneed,long long exp,Matrix* ps,Matrix* flg,PointSet* wal,const Point& pl,const Point& dor);
};
// 当前状态、待计算点、当前经验值
long long calExpr(const State& s,const Point& p,long long exp);
void aStar(const State& s,PointSet& path);
#endif
