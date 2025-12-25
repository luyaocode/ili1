#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <cstdint>
#include <termios.h>
#include <unistd.h>
#include <csignal>
#include <sys/ioctl.h>
#include <math.h>
#include <vector>
#include <ctime>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <queue>
#include <stack>
#include <set>
#include <memory>
#include <new>

#include "d.h"
#include "b.h"
using namespace std;

vector<vector<int>> pos(GRID_SIZE,vector<int>(GRID_SIZE));  // 方格分数数组
vector<vector<int>> flag(GRID_SIZE,vector<int>(GRID_SIZE)); // 方格标识数组
vector<Point> g_wall;                                        // 墙体数组
int player_row = GRID_SIZE/2;               // 初始在中间行
int player_col = GRID_SIZE/2;               // 初始在中间列
int player_level = 0;                       // 初始第0层
long long player_expr = 100;                //  经验值
int player_step_remain = STEP_REMAINING;    // 剩余步数
int player_step_total = 0;                  // 已走步数
int door_row = 0;                           // 门初始位置x
int door_col = 0;                           // 门初始位置y
long long  score_need = 0;                  // 通关所需分数
atomic<bool> game_over{false};              // 游戏是否结束
atomic<bool> auto_move{false};              // 是否自动移动

static State s_state;


// 原子变量用于标记程序是否需要退出
std::atomic<bool> g_should_exit(false);

// 信号处理函数
void signal_handler(int signum) {
    switch(signum) {
        case SIGINT:  // Ctrl+C
            break;
        case SIGTERM: // 终止信号
            break;
        case SIGQUIT: //
            break;
        case SIGHUP:  // 终端关闭
            break;
        default:
            return;
    }
    // 设置退出标志
    game_over = true;
}

const State& getCurrState()
{
    s_state = {score_need,player_expr,&pos,&flag,&g_wall,{player_row,player_col},{door_row,door_col}};
    return s_state;
}

void draw_title();
// 计时器
class Timer
{
public:
      static constexpr const int UPDATE_INTERVAL = 10;
public:
  void start()
  {
      initElapsedTimeLevel();
    auto startTime =std::chrono::high_resolution_clock::now();
    while(!game_over)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(UPDATE_INTERVAL));
        auto currentTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> durationTotal =currentTime-startTime;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_dElapsedTimeTotal = durationTotal.count();
            std::chrono::duration<double> durationLevel =currentTime-m_LevelTimePoint;
            m_dElapsedTimeLevel = durationLevel.count();
            if(DESC_TIME-m_dElapsedTimeLevel<=0)
            {
                m_dElapsedTimeLevel = DESC_TIME;
                game_over =true;
            }
        }
        draw_title();
    }
  }
  void initElapsedTimeLevel()
  {
      std::lock_guard<std::mutex> lock(m_Mutex);
      m_dElapsedTimeLevel = 0.0;
      m_LevelTimePoint = std::chrono::high_resolution_clock::now();
  }
  double getTotalDuration()
  {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_dElapsedTimeTotal;
  }
  double getLevelDuration()
  {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return DESC_TIME - m_dElapsedTimeLevel;
  }
private:
  mutex m_Mutex;
  double m_dElapsedTimeTotal = 0.0;
  double m_dElapsedTimeLevel = 0.0;
  std::chrono::high_resolution_clock::time_point m_LevelTimePoint;
} timer;

// 函数：禁用终端回显和缓冲（实现实时按键监听，无需按回车）
void disable_terminal_echo() {
    struct termios attr;
    tcgetattr(STDIN_FILENO, &attr);    // 获取当前终端属性
    attr.c_lflag &= ~(ICANON | ECHO);  // 关闭规范模式（无需回车）和回显（不显示按键）
    tcsetattr(STDIN_FILENO, TCSANOW, &attr);  // 立即应用新属性
}

// 函数：恢复终端默认设置（程序退出时调用，避免终端异常）
void restore_terminal() {
    struct termios attr;
    tcgetattr(STDIN_FILENO, &attr);
    attr.c_lflag |= (ICANON | ECHO);   // 恢复规范模式和回显
    tcsetattr(STDIN_FILENO, TCSANOW, &attr);
}

// 函数：清空终端屏幕（Linux 通用命令）
void clear_screen() {
    printf("\033[3;H\033[2J");  // 转义序列：光标移到左上角（H）+ 清空屏幕（2J）
    fflush(stdout);            // 强制刷新输出缓冲区，确保屏幕立即清空
}

// 隐藏光标
void hideCursor()
{
    printf("\033[?25l");
    fflush(stdout);
}

// 显示光标
void showCursor()
{
    printf("\033[?25h");
    fflush(stdout);
}

// 评级
const char* coment()
{
    if(player_level>=1000)
    {
        return TIER_GOD;
    }
    else if(player_level>=LEVEL_DESIGN)
    {
        return TIER_EPIC;
    }
    else if(player_level>=LEVEL_DESIGN*0.8)
    {
        return TIER_SOLID;
    }
    else if(player_level>=LEVEL_DESIGN*0.5)
    {
        return TIER_SOSO;
    }
    else if(player_level>=LEVEL_DESIGN*0.2)
    {
        return TIER_WEAK;
    }
    else
    {
        return TIER_NOOB;
    }
}

// 计算难度系数：[1,1000] ，1-100指数增长,100-1000线性增长
double calHardFactor()
{
    int calLevel = player_level;
    if(player_level>LEVEL_DESIGN_MAX)
    {
        return HARD_LEVEL_MAX;
    }
    else if(player_level>LEVEL_DESIGN)
    {
        calLevel = LEVEL_DESIGN + (player_level/LEVEL_DESIGN-1); // 100-200
        double ratio = (double)calLevel / LEVEL_DESIGN; // 1-2
        return HARD_LEVEL_POW*5 + (HARD_LEVEL_MAX-HARD_LEVEL_POW*5)*(ratio-1); // 100-1000
    }
    double ratio = (double)calLevel / LEVEL_DESIGN; // 0-1
    return pow(HARD_LEVEL_POW, ratio); // 0-100
}

// 以概率y将x转为0-9的随机数
// 以概率y将x变为负数
void randTrans(int& x, double y) {
    y = max(0.0, min(1.0, y));
    if ((double)rand() / RAND_MAX < y) {
        x = rand() % MODE_BASE;
    }
    if ((double)rand() / RAND_MAX < y) {
        if(x>10)
        {
            x=0 - x;
        }
    }
}

// 生成随机点
void setRandPoint(Point& p)
{
    int row = rand()%GRID_SIZE;
    int col= rand()%GRID_SIZE;
    while(row==player_row&&col==player_col)
    {
        row = rand()%GRID_SIZE;
        col= rand()%GRID_SIZE;
    }
    p=Point(row,col);
}

// 计算欧式距离
int calDistance(const Point& a,const Point& b)
{
    return pow(abs(a.x-b.x),2)+pow(abs(a.y-b.y),2);
}

// 设置门位置
void setDoorPos(int& row,int& col)
{
    double hardFactor = calHardFactor(); // 1-1000
    double base1Distance = (double)pow(GRID_SIZE/2,2)*2; // 32
    double base3Distance = (double)pow(GRID_SIZE/8,2)*2; // 2
    double distance2Max = min(base1Distance + hardFactor*base3Distance,(double)DISTANCE_MAX);
    double distance2min = min(DISTANCE_MAX_MIN*(hardFactor/HARD_LEVEL_MAX),(double)DISTANCE_MAX_MIN);
    vector<Point> points;
    for (int i = 0; i < GRID_SIZE; ++i) {
        for (int j = 0; j < GRID_SIZE; ++j) {
            if (i == player_row && j == player_col) {
                continue;
            }
            double disCurr2 = (double)calDistance(Point(player_row,player_col),Point(i,j));
            if (disCurr2 >= distance2min && disCurr2 <= distance2Max) {
                points.emplace_back(i, j);
            }
        }
    }
    if(points.empty())
    {
        row = rand()%GRID_SIZE;
        col = rand()%GRID_SIZE;
    }
    else
    {
        int index = rand()%points.size();
        row=points[index].x;
        col=points[index].y;
    }
}

// 计算下一层所需分数
int calScoreNextLevel()
{
    double hardFactor = calHardFactor();
    return ((rand()%((player_level+1)*10)) + player_level)*hardFactor/10;
}

// 计算方格分数
int calPosScore(int r,int c)
{
    double hardFactor = calHardFactor(); // 1->1000
    double changeFactor = calHardFactor()/HARD_LEVEL_MAX/10; // 0-> 0.01 ->0.1
    unsigned long long uniqueKey=(unsigned long long )r*(GRID_SIZE + 1) + c;
    unsigned int hash = (unsigned int )((uniqueKey>>32)^uniqueKey);
    hash = hash* 1103515245 +12345;
    int scoreRange = MAX_SCORE - MIN_SCORE +1;
    int finalScore = (int)(MIN_SCORE + (hash%scoreRange))*(hardFactor)*0.1;
    randTrans(finalScore,changeFactor);
    if(finalScore==0)
    {
        finalScore = r*c+1;
    }
    return finalScore;
}

// 计算墙体位置
// 点a，点b，是否凸型，墙体
void calWallPos(const Point& a,const Point& b,bool isConvex,vector<Point>& walls)
{
    Point p1,p2;
    if(a==b)
    {
        walls.emplace_back(a);
        return ;
    }
    if(a.x<b.x)
    {
        p1=a;
        p2=b;
    }
    else
    {
        p1=b;
        p2=a;
    }

    if(isConvex)
    {
        int highY = p1.y>p2.y?p1.y:p2.y;
        // 横向
        for(int i=p1.x;i<=p2.x;++i)
        {
            walls.emplace_back(i,highY);
        }
        // 纵向
        if(p1.y<p2.y)
        {
            for(int j=p1.y;j<=p2.y;++j)
            {
                walls.emplace_back(p2.x,j);
            }
        }
        else if(p1.y>p2.y)
        {
            for(int j=p2.y;j<=p1.y;++j)
            {
                walls.emplace_back(p1.x,j);
            }
        }
    }
    else
    {
        int lowY = p1.y>p2.y?p2.y:p1.y;
        // 横向
        for(int i=p1.x;i<=p2.x;++i)
        {
            walls.emplace_back(i,lowY);
        }
        // 纵向
        if(p1.y<p2.y)
        {
            for(int j=p1.y;j<=p2.y;++j)
            {
                walls.emplace_back(p1.x,j);
            }
        }
        else if(p1.y>p2.y)
        {
            for(int j=p2.y;j<=p1.y;++j)
            {
                walls.emplace_back(p2.x,j);
            }
        }
    }
}

// BFS判断两点是否联通
bool isConnected(const Point& a,const Point& b,const vector<Point>& walls)
{
    if(a==b) return true;
    if(walls.empty()) return true;
    if(!a.isValid()||!b.isValid()) return false;
    vector<vector<bool>> access(GRID_SIZE,vector<bool>(GRID_SIZE,true));
    for(const auto& w:walls)
    {
        access[w.x][w.y]=false;
    }
    vector<vector<bool>> visited(GRID_SIZE,vector<bool>(GRID_SIZE,false));
    queue<Point> que;
    que.push(a);
    visited[a.x][a.y]=true;
    while(!que.empty())
    {
        Point curr = que.front();
        que.pop();
        if(curr==b)
        {
            return true;
        }
        for(const auto& dir:DIRECTIONS)
        {
            Point np = curr+dir;
            if(np.isValid()&&access[np.x][np.y]&&!visited[np.x][np.y])
            {
                visited[np.x][np.y]=true;
                que.push(np);
            }
        }
    }
    return false;
}

// bfs
void bfs(const Point& a,const Point& b,Path& currPath)
{
    if(a==b) return;
    if(!a.isValid()||!b.isValid()) return;
    vector<vector<bool>> access(GRID_SIZE,vector<bool>(GRID_SIZE,true));
    for(const auto& w:g_wall)
    {
        access[w.x][w.y]=false;
    }
    vector<vector<bool>> visited(GRID_SIZE,vector<bool>(GRID_SIZE,false));
    queue<Point*> que;
    char buffer[sizeof(Point)*GRID_SIZE*GRID_SIZE];
    Point* offset = new (buffer) Point(a);
    que.push(offset);
    offset +=1;
    visited[a.x][a.y]=true;
    Point* tail = nullptr;
    while(!que.empty())
    {
        Point* curr = que.front();
        que.pop();
        if(*curr==b)
        {
            tail = curr;
            break;
        }
        for(const auto& dir:DIRECTIONS)
        {
            Point tt = *curr+dir;
            if(tt.isValid()&&access[tt.x][tt.y]&&!visited[tt.x][tt.y])
            {
                Point* np = new (offset) Point(tt);
                offset+=1;
                visited[np->x][np->y]=true;
                que.push(np);
                np->prev=curr;
            }
        }
    }
    while(tail&&(*tail!=a))
    {
        currPath.push_back({tail->x,tail->y});
        tail = tail->prev;
    }
    currPath.push_back(a);
    reverse(currPath.begin(),currPath.end());
}

// 贪心
void greedy(const Point& a,const Point& b, Path& currPath)
{
    vector<vector<bool>> access(GRID_SIZE,vector<bool>(GRID_SIZE,true));
    vector<vector<bool>> visited(GRID_SIZE,vector<bool>(GRID_SIZE,false));
    for(const auto& w:g_wall)
    {
        access[w.x][w.y]=false;
    }
    stack<Point> q;
    q.push(a);
    visited[a.x][a.y] = true;
    auto currExpr = player_expr;
    while(!(q.top() == b))
    {
        auto tmpexpr = currExpr;   // 保存
        long long  expmax = INT64_MIN;
        Point tmp(-1,-1);
        for(const auto& dir:DIRECTIONS)
        {
            Point curr = q.top();
            Point np = curr+dir;
            if(!np.isValid()||!access[np.x][np.y]||visited[np.x][np.y])
            {
                continue;
            }
//            State s(score_need,player_expr,&pos,&flag,&g_wall,{player_row,player_col},{door_row,door_col});
            const State& s = getCurrState();
            auto expr = calExpr(s,np,currExpr);
            expmax = max(expmax,expr);
            if(expmax == expr)
            {
                tmp = np;
                tmpexpr = expr;
            }
        }
        if(tmp==Point(-1,-1))
        {
            auto front = q.top();
            q.pop();
            visited[front.x][front.y]=true;
            for(const auto& dir:DIRECTIONS)
            {
                Point np = front+dir;
                if(np.isValid()&&access[np.x][np.y])
                {
                    visited[np.x][np.y]=true;
                }
            }
        }
        else
        {
            q.push(tmp);
            visited[tmp.x][tmp.y] = true;
            currExpr = tmpexpr;
        }
    }
    while(!q.empty())
    {
        currPath.push_back(q.top());
        q.pop();
    }
    reverse(currPath.begin(),currPath.end());
}

// `：计算可达路径
// 输入：起点，终点，可达表，访问表
// 输出：当前路径，路径列表
// 返回：无
void dfs(const Point& a,const Point& b,const vector<vector<bool>>& access,vector<vector<bool>>& visited,Path& currPath,vector<Path>& paths)
{
    if((!a.isValid()) || (!b.isValid()))
    {
        return;
    }
    if(!access[a.x][a.y] || visited[a.x][a.y])
    {
        return;
    }
    visited[a.x][a.y]=true;
    currPath.emplace_back(a);
    if(a==b)
    {
        paths.push_back(currPath);
    }
    else
    {
        for(const auto& dir:DIRECTIONS)
        {
            Point np = a+dir;
            if(calDistance(np,b)<calDistance(a,b))
            {
               dfs(np,b,access,visited,currPath,paths);
            }
        }
    }
    currPath.pop_back();
    visited[a.x][a.y]=false;
}

// 计算最优路径
// 输入：起点，终点
// 输出：路径
// 返回：路径长度 ,-1 表示无路径
int calBestPath(const Point& a,const Point& b,Path& path)
{
    Path ptGreedy;
    greedy(a,b,ptGreedy);
    Path ptBfs;
    bfs(a,b,ptBfs);
    if(player_step_remain >= (int)ptGreedy.size()-1 || player_level<10)
    {
        path = ptGreedy;
    }
    else if(player_step_remain >= (int)ptBfs.size()-1)
    {
        path = ptBfs;
    }

    return path.size()-1;
}

// 生成随机状态
int genFlag(int r,int c)
{
    if(abs(pos[r][c])<2) {
        return 0;
    }
    int hardFactor = calHardFactor()*0.009 + 1; // 1-10
    if(abs(pos[r][c])<(10/hardFactor))
    {
        if(rand()%(GRID_SIZE)>(GRID_SIZE/5))
        {
            return INT_DEVISION;
        }
        else
        {
            return INT_MULTIPLY;
        }
    }
    return 0;
}

// 生成随机墙体
void genWall(vector<Point>& walls)
{
    double hardFactor = calHardFactor()/HARD_LEVEL_MAX; // 0.001-1
    int nWall = (int)(1+hardFactor*3);
    for(int i=0;i<nWall;++i)
    {
        Point a,b;
        setRandPoint(a);
        setRandPoint(b);
        double maxdis = min(DISTANCE_MAX/3*(1+hardFactor),(double)DISTANCE_MAX/2*(1+hardFactor));
        while(calDistance(a,b)>maxdis)
        {
            setRandPoint(a);
            setRandPoint(b);
        }
        calWallPos(a,b,rand()%2,walls);
    }
    // 去重
    sort(walls.begin(),walls.end());
    auto last = std::unique(walls.begin(),walls.end());
    walls.erase(last,walls.end());
}

// 删除列表某个点
void removePoint(const Point& point,vector<Point>& vec)
{
    auto it =std::find(vec.begin(),vec.end(),point);
    if(it!=vec.end())
    {
        vec.erase(it);
    }
}

// 初始化网格
void initScreen()
{
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            pos[r][c]= calPosScore(r,c);
        }
    }
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            flag[r][c] = genFlag(r,c);
        }
    }
    vector<Point> walls;
    genWall(walls);
    removePoint({player_row,player_col},walls);
    setDoorPos(door_row,door_col);
    removePoint({door_row,door_col},walls);
    int retry = 3;
    while(!isConnected({player_row,player_col},{door_row,door_col},walls)&&retry>0)
    {
        setDoorPos(door_row,door_col);
        removePoint({door_row,door_col},walls);
        retry--;
    }
    if(retry==0)
    {
        while(!isConnected({player_row,player_col},{door_row,door_col},walls))
        {
            Point t;
            setRandPoint(t);
            door_row = t.x;
            door_col = t.y;
            removePoint({door_row,door_col},walls);
        }
    }
    for(auto& w:walls)
    {
        flag[w.x][w.y]= INT_WALL;
    }
    flag[door_row][door_col]=INT_NOTHING;
    score_need = calScoreNextLevel();
    pos[door_row][door_col]=score_need;
    pos[player_row][player_col]=INT_NOTHING;
    //
    g_wall = walls;
}


void int2char(int val,char* c)
{
    if(val==0)
    {
        memcpy(c,POS_WALL,NUM_SIZE+1);
    }
    else
    {
        char num_str[10];
        snprintf(num_str,sizeof(num_str),"%+d",val);
        int num_len = strlen(num_str);
        int space_total = NUM_SIZE-num_len;
        int space_left = (space_total+1)/2;
        int space_right = space_total-space_left;
        c[0]='\0';
        for(int i=0;i<space_left;++i)
        {
            strcat(c," ");
        }
        strcat(c,num_str);
        for(int i=0;i<space_right;++i)
        {
            strcat(c," ");
        }
    }
}

void flag2char(int flag,char*c)
{
    switch(flag)
    {
    case INT_ARROW_LEFT:
        memcpy(c,ARROW_LEFT,NUM_SIZE+1);
        break;
    case INT_ARROW_UP:
        memcpy(c,ARROW_UP,NUM_SIZE+1);
        break;
    case INT_ARROW_RIGHT:
        memcpy(c,ARROW_RIGHT,NUM_SIZE+1);
        break;
    case INT_ARROW_DOWN:
        memcpy(c,ARROW_DOWN,NUM_SIZE+1);
        break;
    default:
        break;
    }
}

// 替换字符
void replaceStr(char* c,char src,char tar)
{
    int len = strlen(c);
    for(int i=0;i<len;++i)
    {
        if(c[i]==src)
        {
           c[i]=tar;
           break;
        }
    }
}

// 数值+状态转为字符串
void intflag2char(int val,int flag,char* c)
{
    int2char(val,c);
    switch (flag) {
    case INT_MULTIPLY:
    {
        replaceStr(c,'+','x');
        replaceStr(c,'-','x');
        break;
    }
    case INT_DEVISION:
    {
        replaceStr(c,'+','/');
        replaceStr(c,'-','/');
        break;
    }
    default:
        break;
    }
}

// 绘制网格
void draw_grid() {
    for (int r = 0; r < GRID_SIZE; r++) {
        for (int c = 0; c < GRID_SIZE; c++) {
            char tmp[NUM_SIZE+1];
            memset(tmp,0,sizeof(tmp));
            if(r==player_row&&c==player_col)
            {
                memcpy(tmp,POS_PLAYER,NUM_SIZE+1);
            }
            else if(r==door_row&&c==door_col)
            {
                memcpy(tmp,POS_DOOR,NUM_SIZE+1);
            }
            else if(pos[r][c]==0&&flag[r][c]<0&&flag[r][c]>=INT_ARROW_DOWN)
            {
                flag2char(flag[r][c],tmp);
            }
            else if(flag[r][c]<=INT_MULTIPLY&&flag[r][c]>=INT_DEVISION)
            {
                intflag2char(pos[r][c],flag[r][c],tmp);
            }
            else if(flag[r][c]==INT_WALL)
            {
                memcpy(tmp,POS_WALL,NUM_SIZE+1);
            }
            else
            {
                int2char(pos[r][c],tmp);
            }
            printf("%s",tmp);
        }
        printf("\n\n");
    }
}

// 绘制计时块
void draw_clock()
{
    printf("total duration: %.3f\t",timer.getTotalDuration());
    printf("level duration: %.3f\t",timer.getLevelDuration());
    printf("steps: %d\n",player_step_remain);
}

// 绘制标题块
void draw_title(){
    printf("\033[1;1H");
//    printf("===Undefined===\n");
    printf("=== move: ↑ ↓ ← → exit: ESC auto: M === \n\n");
    draw_clock();
}

// 绘制经验值块
void draw_expr(){
    printf("level: %d\t",player_level);
    printf("expr：%lld\t",player_expr);
    printf("target: %d\n\n",pos[door_row][door_col]);
}

// 函数：监听按键（上下左右键 + ESC 键）
int get_key() {
    int ch = getchar();  // 读取按键 ASCII 码
    // 上下左右键是特殊键，需处理转义序列（格式：ESC [ A/B/C/D）
    if (ch == 27) {  // 先判断是否是 ESC 键（ASCII 27）
        // 读取后续字符，判断是否是方向键的转义序列
        if (getchar() == '[') {
            switch (getchar()) {
                case 'A': return 'U';
                case 'B': return 'D';
                case 'C': return 'R';
                case 'D': return 'L';
            }
        } else {
            return 'E';
        }
    }
    return ch;
}

// 更新历史轨迹方向
void update_arrow_road(int key)
{
    switch (key) {
        case 'U':
    {
        flag[player_row][player_col] = -2;
            break;
    }
        case 'D':
    {                   flag[player_row][player_col] = -4;
            break;
    }
        case 'L':
    {                   flag[player_row][player_col] = -1;
            break;
    }
        case 'R':
    {                    flag[player_row][player_col] = -3;
            break;
    }
    default:
        break;
    }
}

void update_expr(int r,int c,long long& expr)
{
    switch(flag[r][c])
    {
    case INT_MULTIPLY:
        expr*=abs(pos[r][c]);
        break;
    case INT_DEVISION:
    {
        if(pos[r][c]!=0)
        {
           expr/=abs(pos[r][c]);
        }
        break;
    }
    default:
        expr+=pos[r][c];
        break;
    }
}

// 计算从a到b的按键操作
int calKey(const Point& a,const Point& b)
{
    if(abs(a.x-b.x)==1)
    {
        if(a.x<b.x)
            return 'D';
        else
            return 'U';
    }
    if(abs(a.y-b.y)==1)
    {
       if(a.y<b.y)
           return 'R';
       else
           return 'L';
    }
    return 0;
}

// 自动模式
void autoMove(vector<int>& keys)
{
    Path path;
    int step = calBestPath({player_row,player_col},{door_row,door_col},path);
    if(step<0)
    {
        return ;
    }
    Point curr = {player_row,player_col};
    for(auto const& p:path)
    {
        if(p==curr)
        {
            continue;
        }
        int key = calKey(curr,p);
        curr=p;
        keys.push_back(key);
    }
}

// 更新player信息：坐标、经验值等
void update_player(int key,bool& ok) {
    int tRow=player_row,tCol=player_col;
    long long  tExpr =player_expr;
    switch (key) {
        case 'U':
    {
            if (player_row > 0) tRow--;
            else return;
            break;
    }
        case 'D':
    {        if (player_row < GRID_SIZE - 1) tRow++;
            else return;
            break;
    }
        case 'L':
    {       if (player_col > 0) tCol--;
            else return;
            break;
    }
        case 'R':
    {        if (player_col < GRID_SIZE - 1) tCol++;
            else return;
            break;
    }
    default:
        break;

    }
    bool onDoor = false;
    if(tRow==door_row&&tCol==door_col)
    {
        tExpr-=pos[door_row][door_col];
        onDoor =true;
    }
    else
    {
        update_expr(tRow,tCol,tExpr);
    }
    // 墙体
    if(flag[tRow][tCol]==INT_WALL)
    {
        return;
    }
    if(tExpr>=0)
    {
        update_arrow_road(key);
        pos[player_row][player_col]=0;
        player_row=tRow;
        player_col=tCol;
        player_expr=tExpr;
    }
    else
    {
        return;
    }
    player_step_remain--;
    if(onDoor)
    {
        initScreen();
        player_level++;
        timer.initElapsedTimeLevel();
        player_step_remain+=STEP_STEP_ADD;
    }
    if(player_step_remain<=0)
    {
        game_over =true; // 贪心、屏蔽
    }
    ok =true;
}

void draw()
{
    clear_screen();
    draw_title();
    draw_expr();
    draw_grid();
}

void initSys()
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGQUIT, signal_handler);
    signal(SIGHUP, signal_handler);
}

int main(int argc,char** argv) {
    int seed = 0x0;
    if(argc>1)
    {
        seed = stoi(argv[1]);
    }
    else
    {
        seed = (unsigned long)time(NULL);
    }
    srand(seed);
    initSys();
    disable_terminal_echo();
    hideCursor();
    initScreen();
    draw();
    std::thread thread_timer(&Timer::start,&timer);
    thread_timer.detach();

    while (!game_over) {
        int key;
        if(!auto_move)
        {
           key = get_key();
           if (key == 'E') break;
           if(key=='M')
           {
               auto_move = true;
               continue;
           }
           bool ok{false};
           update_player(key,ok);
           if(ok)
           {
               draw();
           }
        }
        else
        {
            vector<int> keys;
            autoMove(keys);
            if(keys.empty())
            {
                game_over=true;
                break;
            }
            for(const auto& key:keys)
            {
                this_thread::sleep_for(std::chrono::milliseconds(100));
                bool ok{false};
                update_player(key,ok);
                if(ok)
                {
                    draw();
                }
                if(game_over)
                {
                    break;
                }
            }
        }
    }

    // 退出前恢复终端设置 + 清空屏幕
    restore_terminal();
    printf("Your level: %d (%s)\n",player_level,coment());
    printf("Time: %.3f s\n",timer.getTotalDuration());
    if(thread_timer.joinable())
    {
        thread_timer.join();
    }
    showCursor();
    return 0;
}
