#ifndef BALLWINDOW_H
#define BALLWINDOW_H
#include "inc.h"
#include "ball.h"

// 小球数据
struct BallData {
    quint32 id;        // 小球ID
    float x, y;        // 位置
    float vx, vy;      // 速度
    float radius;      // 半径
    float rotation;    // 旋转角度
    float rotationSpeed; // 旋转速度
};

const int MAX_WINDOWS = 10;  // 最大窗口数量
const int MAX_BALLS = 20;    // 每个窗口最大小球数量
const int CONNECT_MIN = 10;

struct WindowInfo {
    int pid;               // 进程ID
    int x, y;              // 窗口位置
    int width, height;     // 窗口大小
    int ballCount;         // 小球数量

    BallData balls[MAX_BALLS];
};

// 共享内存数据结构
struct SharedWindowData {
    int windowCount;  // 当前活动窗口数量
    // 窗口信息结构
    WindowInfo windows[MAX_WINDOWS];
};

// 主窗口类
class BallWindow : public QWidget {
    Q_OBJECT

public:
    BallWindow(QWidget *parent = nullptr);

    ~BallWindow();

protected:
    void paintEvent(QPaintEvent *event) override;
    // 鼠标按下时：记录拖拽起点
    void mousePressEvent(QMouseEvent *event) override;

    // 鼠标移动时：计算新位置并移动窗口
    void mouseMoveEvent(QMouseEvent *event) override;

    // 鼠标释放时：结束拖拽状态
    void mouseReleaseEvent(QMouseEvent *event) override;

    // 重写鼠标双击事件：双击窗口任意位置关闭窗口
    void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
    void updateSimulation();

    // 更新共享内存数据
    void updateSharedMemory(bool isRemoving = false);

private:
    // 注册信号处理函数（绑定到全局处理函数）
    void registerSignalHandlers();

    // 全局信号处理函数（转发到实例的 onSignal）
    static void globalSignalHandler(int signum);

    // 信号处理的成员函数（实际执行清理逻辑）
    void onSignal(int signum);
    // 获取其他窗口信息
   QVector<WindowInfo> getOtherWindows();

   // 接收从其他窗口进入的小球
   void receiveBallsFromOtherWindows(const QVector<WindowInfo>& otherWindows);

   // 检查是否已有相同ID的小球
   bool hasBallWithId(quint32 id) const;

   // 辅助函数：从当前窗口移除指定ID的小球（当小球完全进入另一个窗口时）
   void removeBallWithId(int targetId);

   // 判断点是否在右侧窗口中
   bool isBallInRightWindow(const Ball& ball, const QRect& thisRect, const QVector<WindowInfo>& otherWindows);
   // 判断点是否在左侧窗口中
   bool isBallInLeftWindow(const Ball& ball, const QRect& thisRect, const QVector<WindowInfo>& otherWindows);
   // 判断点是否在上侧窗口中
   bool isBallInTopWindow(const Ball& ball, const QRect& thisRect, const QVector<WindowInfo>& otherWindows);
   // 判断点是否在下侧窗口中
   bool isBallInBottomWindow(const Ball& ball, const QRect& thisRect, const QVector<WindowInfo>& otherWindows);

private:
   // 静态成员：保存当前 BallWindow 实例指针
   static BallWindow* s_instance;
   QRectF m_bounds;                  // 窗口内的正方形区域
   std::vector<Ball> m_balls;        // 小球集合
   QTimer* m_timer;                  // 模拟定时器
   QTimer* m_shareTimer;             // 共享内存更新定时器
   QSharedMemory m_sharedMemory;     // 用于进程间通信的共享内存
   bool m_isDragging;       // 标记是否正在拖拽窗口
   QPoint m_dragStartPos;   // 鼠标按下时相对于窗口左上角的位置
   QPoint m_lastWindowPos;  // 记录上一时刻窗口位置
   QElapsedTimer m_windowMoveTimer;  // 计算窗口移动时间
   QPointF m_windowVelocity;  // 窗口移动速度（像素/毫秒）
};


#endif // BALLWINDOW_H
