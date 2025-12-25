#include "ballwindow.h"

// 初始化静态成员
BallWindow* BallWindow::s_instance = nullptr;

BallWindow::BallWindow(QWidget *parent) : QWidget(parent), m_sharedMemory("BallWindowsSharedMemory") {

    // 构造时将当前实例指针保存到全局变量
    s_instance = this;
    // 注册信号处理函数
    registerSignalHandlers();

    // 设置窗口属性
    setWindowTitle("小球碰撞盒");
    // 关键：一次性设置多个窗口标志（用 | 组合）
    setWindowFlags(
        Qt::FramelessWindowHint    // 无边框
        | Qt::WindowStaysOnTopHint // 窗口置顶
    );

    setFixedSize(400, 400);
    m_bounds = QRectF(0, 0, width(), height());

    // 初始化共享内存
    if (!m_sharedMemory.create(sizeof(SharedWindowData))) {
        QSharedMemory::SharedMemoryError error = m_sharedMemory.error();
        if (error == QSharedMemory::AlreadyExists) {
            qDebug() << "共享内存已存在，尝试附加 - key:" << m_sharedMemory.nativeKey();
            // 尝试附加到已存在的共享内存
            if (!m_sharedMemory.attach()) {
                qWarning("附加到共享内存失败 - 错误描述: %s",
                         qPrintable(m_sharedMemory.errorString()));
            } else {
                qDebug("成功附加到已存在的共享内存");
            }
        } else {
            // 其他错误（非“已存在”）
            qWarning("无法创建共享内存 - 错误类型: %d, 错误描述: %s",
                     error,
                     qPrintable(m_sharedMemory.errorString()));
        }
    } else {
        qDebug("成功创建新的共享内存");
    }

    // 创建初始小球
    if(getOtherWindows().size() == 0)
    {
        m_balls.emplace_back(m_bounds);
    }

    // 设置定时器更新
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &BallWindow::updateSimulation);
    m_timer->start(16);  // 约60FPS

    // 定期更新共享内存
    m_shareTimer = new QTimer(this);
    connect(m_shareTimer, &QTimer::timeout, this,[this](){
        updateSharedMemory();
    });
    // 延迟16ms后启动，初始触发时间错开
    QTimer::singleShot(16, this, [this](){
        m_shareTimer->start(100);
    });

    // 初始化窗口位置记录
    m_lastWindowPos = pos();
    m_windowMoveTimer.start();
}

BallWindow::~BallWindow() {
    // 正常析构时执行清理
    updateSharedMemory(true);
    // 清除全局实例指针（避免悬空）
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void BallWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制背景
    painter.fillRect(rect(), Qt::white);

    // 绘制边框
    painter.setPen(QPen(Qt::black, 2));
    painter.drawRect(m_bounds.adjusted(0, 0, -1, -1));

    // 绘制小球
    for (const auto& ball : m_balls) {
        painter.save();

        // 移动到小球中心并旋转
        painter.translate(ball.position());
        painter.rotate(ball.rotation());

        // 绘制小球（带一些图案以便观察旋转）
        painter.setBrush(Qt::blue);
        painter.drawEllipse(QPointF(0, 0), ball.radius(), ball.radius());
        painter.setPen(Qt::white);
        painter.drawLine(0, 0, ball.radius(), 0);

        painter.restore();
    }
}

void BallWindow::mousePressEvent(QMouseEvent *event) {
    // 仅处理左键点击，且点击区域为窗口客户区（可根据需求限制拖拽区域，如顶部区域）
    if (event->button() == Qt::LeftButton) {
        // 拖拽起点
        m_dragStartPos = event->globalPos() - frameGeometry().topLeft();
        m_isDragging = true;
        // 捕获鼠标（避免拖拽时鼠标移出窗口导致拖拽中断）
        event->accept();
    }
    QWidget::mousePressEvent(event);
}

void BallWindow::mouseMoveEvent(QMouseEvent *event) {
    // 只有在拖拽状态且左键按住时才处理
    if (m_isDragging && (event->buttons() & Qt::LeftButton)) {
        // 计算窗口新位置：当前鼠标全局位置 - 拖拽起点 = 窗口左上角新位置
        QPoint newPos = event->globalPos() - m_dragStartPos;
        move(newPos); // 移动窗口
        // 计算窗口移动速度（位移 / 时间）
       qint64 elapsed = m_windowMoveTimer.restart();  // 毫秒
       if (elapsed > 0) {  // 避免除零
           QPoint delta = newPos - m_lastWindowPos;  // 位移差
           m_windowVelocity = QPointF(
               static_cast<qreal>(delta.x()) / elapsed,  // x方向速度（像素/毫秒）
               static_cast<qreal>(delta.y()) / elapsed   // y方向速度（像素/毫秒）
           );
       }
       m_lastWindowPos = newPos;  // 更新上一位置
       event->accept();
    }
    QWidget::mouseMoveEvent(event);
}

void BallWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        m_windowVelocity = QPointF(0, 0);  // 停止拖拽后窗口速度为0
    }
    QWidget::mouseReleaseEvent(event);
}

void BallWindow::mouseDoubleClickEvent(QMouseEvent *event) {
    // 确保事件被正确处理（可限定左键双击，按需求调整）
    if (event->button() == Qt::LeftButton) {
        qDebug() << "鼠标双击，关闭窗口...";
        close(); // 调用关闭逻辑
    }
    // 传递事件给父类（可选，根据需要是否保留默认行为）
    QWidget::mouseDoubleClickEvent(event);
}

void BallWindow::updateSimulation() {
    // 读取共享内存获取其他窗口信息
    QVector<WindowInfo> otherWindows = getOtherWindows();

    // 检测连接状态（相邻窗口）
    bool leftConnected = false, rightConnected = false;
    bool topConnected = false, bottomConnected = false;

    // 检查每个窗口是否相邻
    const QRect thisRect = geometry();
    for (const auto& win : otherWindows) {
        QRect otherRect(win.x, win.y, win.width, win.height);

        // 检查右侧连接
        if (std::abs(thisRect.right() - otherRect.left()) < CONNECT_MIN &&
                thisRect.intersects(QRect(otherRect.x(), otherRect.y(), 1, otherRect.height()))) {
            rightConnected = true;
        }

        // 检查左侧连接
        if (std::abs(otherRect.right() - thisRect.left()) < CONNECT_MIN &&
                thisRect.intersects(QRect(otherRect.x() + otherRect.width(), otherRect.y(), 1, otherRect.height()))) {
            leftConnected = true;
        }

        // 检查顶部连接
        if (std::abs(otherRect.bottom() - thisRect.top()) < CONNECT_MIN &&
                thisRect.intersects(QRect(otherRect.x(), otherRect.y() + otherRect.height(), otherRect.width(), 1))) {
            topConnected = true;
        }

        // 检查底部连接
        if (std::abs(thisRect.bottom() - otherRect.top()) < CONNECT_MIN &&
                thisRect.intersects(QRect(otherRect.x(), otherRect.y(), otherRect.width(), 1))) {
            bottomConnected = true;
        }
    }

    // 更新小球位置
    for (auto& ball : m_balls) {
        ball.update(leftConnected, rightConnected, topConnected, bottomConnected, m_windowVelocity);
    }

    // 处理小球离开当前窗口的情况（从共享内存中移除）
    // TODO:检查碰撞点是否在其他窗口中
    auto it = m_balls.begin();
    while (it != m_balls.end()) {
        if (it->isOutOfRight() && rightConnected && isBallInRightWindow(*it,thisRect,otherWindows)) {
            // 小球已离开右侧，由其他窗口接收
            it = m_balls.erase(it);
        }
        else if (it->isOutOfLeft() && leftConnected && isBallInLeftWindow(*it,thisRect,otherWindows)) {
            // 小球已离开左侧
            it = m_balls.erase(it);
        }
        else if (it->isOutOfTop() && topConnected && isBallInTopWindow(*it,thisRect,otherWindows)) {
            // 小球已离开顶部
            it = m_balls.erase(it);
        }
        else if (it->isOutOfBottom() && bottomConnected && isBallInBottomWindow(*it,thisRect,otherWindows)) {
            // 小球已离开底部
            it = m_balls.erase(it);
        }
        else {
            ++it;
        }
    }

    // 接收从其他窗口进入的小球
    receiveBallsFromOtherWindows(otherWindows);

    update();
}

void BallWindow::updateSharedMemory(bool isRemoving) {
    if (!m_sharedMemory.lock()) return;

    SharedWindowData* data = static_cast<SharedWindowData*>(m_sharedMemory.data());
    if(nullptr == data)
    {
        return;
    }
    int currentPid = QApplication::applicationPid();
    QRect geo = geometry();

    // 查找当前进程在共享内存中的位置
    int index = -1;
    for (int i = 0; i < data->windowCount; ++i) {
        if (data->windows[i].pid == currentPid) {
            index = i;
            break;
        }
    }

    if (isRemoving) {
        // 从共享内存中移除当前窗口
        if (index != -1) {
            for (int i = index; i < data->windowCount - 1; ++i) {
                data->windows[i] = data->windows[i + 1];
            }
            data->windowCount--;
        }
    } else {
        // 添加或更新当前窗口信息
        if (index == -1 && data->windowCount < MAX_WINDOWS) {
            index = data->windowCount++;
        }

        if (index != -1) {
            // 更新窗口信息
            WindowInfo& winInfo = data->windows[index];
            winInfo.pid = currentPid;
            winInfo.x = geo.x();
            winInfo.y = geo.y();
            winInfo.width = geo.width();
            winInfo.height = geo.height();
            winInfo.ballCount = qMin(static_cast<int>(m_balls.size()), MAX_BALLS);

            // 更新小球信息
            for (int i = 0; i < winInfo.ballCount; ++i) {
                const auto& ball = m_balls[i];
                auto& ballData = winInfo.balls[i];
                ballData.id = ball.id();
                ballData.x = ball.position().x();
                ballData.y = ball.position().y();
                ballData.vx = ball.velocity().x();
                ballData.vy = ball.velocity().y();
                ballData.radius = ball.radius();
                ballData.rotation = ball.rotation();
                ballData.rotationSpeed = ball.rotationSpeed();
            }
        }
    }

    m_sharedMemory.unlock();
}

void BallWindow::registerSignalHandlers() {
    signal(SIGINT, &BallWindow::globalSignalHandler);   // Ctrl+C
    signal(SIGTERM, &BallWindow::globalSignalHandler);  // kill 命令
}

void BallWindow::globalSignalHandler(int signum) {
    if (s_instance != nullptr) {
        // 调用当前实例的信号处理成员函数
        s_instance->onSignal(signum);
    } else {
        // 若实例不存在，直接退出
        _exit(1);
    }
}

void BallWindow::onSignal(int signum) {
    // 执行共享内存清理
    updateSharedMemory(true);
    // 清理完成后退出进程
    _exit(1); // 用 _exit 而非 exit，避免触发析构二次清理
}

QVector<WindowInfo> BallWindow::getOtherWindows() {
    QVector<WindowInfo> result;

    while (!m_sharedMemory.lock())
    {
        QThread::msleep(100);
    }
    SharedWindowData* data = static_cast<SharedWindowData*>(m_sharedMemory.data());
    if(nullptr == data)
    {
        return result;
    }
    int currentPid = QApplication::applicationPid();

    // 收集其他窗口信息
    for (int i = 0; i < data->windowCount; ++i) {
        if (data->windows[i].pid != currentPid) {
            result.push_back(data->windows[i]);
        }
    }
    m_sharedMemory.unlock();
    return result;
}

void BallWindow::receiveBallsFromOtherWindows(const QVector<WindowInfo> &otherWindows)
{
    if(!m_balls.empty())
    {
        return;
    }
    QRect thisRect = geometry();
    for (const WindowInfo& win : otherWindows) {
        QRect otherRect(win.x, win.y, win.width, win.height);

        // 1. 检查是否有小球进入当前窗口左侧
       if (otherRect.right() - thisRect.left() > 0 && otherRect.right() - thisRect.left() < CONNECT_MIN) {
           // 左侧窗口的右边界到当前窗口左边界的距离足够近（视为衔接）
           for (int i = 0; i < win.ballCount; ++i) {
               const BallData& ballData = win.balls[i];
               // 小球从右侧窗口右侧溢出，且未在当前窗口存在
               if (ballData.x + ballData.radius > otherRect.width()  // 小球右边缘超出左侧窗口右边界
                   && !hasBallWithId(ballData.id)) {

                   // 计算进入当前窗口的位置（确保衔接自然，基于相对位置）
                   // 小球在右侧窗口溢出的距离：overflow = (x + radius) - otherRect.width()
                   // 在当前窗口的位置：leftEntryX + overflow（从左边界开始向内）
                   qreal overflow = (ballData.x + ballData.radius) - otherRect.width();
                   qreal newX = /*leftEntryX + */overflow;
                   qreal newY = ballData.y + (otherRect.y() - thisRect.y()); // 修正Y坐标（窗口可能上下错位）

                   // 创建新球并加入当前窗口
                   Ball newBall(m_bounds);
                   newBall.setPosition(QPointF(newX, newY));
                   newBall.setVelocity(QVector2D(ballData.vx, ballData.vy)); // 保持速度一致
                   newBall.setRotation(ballData.rotation);
                   newBall.setRotationSpeed(ballData.rotationSpeed);
                   newBall.setId(ballData.id);

                   m_balls.push_back(newBall);
               }
           }
       }

       // 2. 检查是否有小球进入当前窗口右侧
       if (thisRect.right() - otherRect.left() < CONNECT_MIN) {
           // 当前窗口右边界到左侧窗口左边界的距离足够近（视为衔接）
           for (int i = 0; i < win.ballCount; ++i) {
               const BallData& ballData = win.balls[i];
               // 小球从左侧窗口左侧溢出，且未在当前窗口存在
               if (ballData.x - ballData.radius < 0  // 小球左边缘超出左侧窗口左边界
                   && !hasBallWithId(ballData.id)) {

                   // 计算进入当前窗口的位置（确保衔接自然）
                   // 小球在左侧窗口溢出的距离：overflow = 0 - (x - radius)
                   // 在当前窗口的位置：rightEntryX - overflow（从右边界开始向内）
                   qreal overflow = 0 - (ballData.x - ballData.radius);
                   qreal newX = thisRect.width() - overflow;
                   qreal newY = ballData.y + (otherRect.y() - thisRect.y()); // 修正Y坐标

                   // 创建新球并加入当前窗口
                   Ball newBall(m_bounds);
                   newBall.setPosition(QPointF(newX, newY));
                   newBall.setVelocity(QVector2D(ballData.vx, ballData.vy)); // 保持速度一致
                   newBall.setRotation(ballData.rotation);
                   newBall.setRotationSpeed(ballData.rotationSpeed);
                   newBall.setId(ballData.id);

                   m_balls.push_back(newBall);
               }
           }
       }

       // 3. 检查是否有小球进入当前窗口上侧
      if (otherRect.bottom() - thisRect.top() > 0 && otherRect.bottom() - thisRect.top() < CONNECT_MIN) {
          for (int i = 0; i < win.ballCount; ++i) {
              const BallData& ballData = win.balls[i];
              if (ballData.y + ballData.radius > otherRect.height()  // 小球下边缘超出上侧窗口下边界
                  && !hasBallWithId(ballData.id)) {

                  // 计算进入当前窗口的位置（确保衔接自然，基于相对位置）
                  // 小球在上侧窗口溢出的距离：overflow = (y + radius) - otherRect.height()
                  qreal overflow = (ballData.y + ballData.radius) - otherRect.height();
                  qreal newY = overflow;
                  qreal newX = ballData.x + (otherRect.x() - thisRect.x()); // 修正X坐标（窗口可能上下错位）

                  // 创建新球并加入当前窗口
                  Ball newBall(m_bounds);
                  newBall.setPosition(QPointF(newX, newY));
                  newBall.setVelocity(QVector2D(ballData.vx, ballData.vy)); // 保持速度一致
                  newBall.setRotation(ballData.rotation);
                  newBall.setRotationSpeed(ballData.rotationSpeed);
                  newBall.setId(ballData.id);

                  m_balls.push_back(newBall);
              }
          }
      }

      // 4. 检查是否有小球进入当前窗口下侧
      if (thisRect.bottom() - otherRect.top() < CONNECT_MIN) {
          // 当前窗口下边界到下侧窗口上边界的距离足够近（视为衔接）
          for (int i = 0; i < win.ballCount; ++i) {
              const BallData& ballData = win.balls[i];
              // 小球从下侧窗口上侧溢出，且未在当前窗口存在
              if (ballData.y - ballData.radius < 0  // 小球上边缘超出下侧窗口上边界
                  && !hasBallWithId(ballData.id)) {

                  // 计算进入当前窗口的位置（确保衔接自然）
                  // 小球在下侧窗口溢出的距离：overflow = 0 - (y - radius)
                  qreal overflow = 0 - (ballData.y - ballData.radius);
                  qreal newY = thisRect.height() - overflow;
                  qreal newX = ballData.x + (otherRect.x() - thisRect.x()); // 修正X坐标

                  // 创建新球并加入当前窗口
                  Ball newBall(m_bounds);
                  newBall.setPosition(QPointF(newX, newY));
                  newBall.setVelocity(QVector2D(ballData.vx, ballData.vy)); // 保持速度一致
                  newBall.setRotation(ballData.rotation);
                  newBall.setRotationSpeed(ballData.rotationSpeed);
                  newBall.setId(ballData.id);

                  m_balls.push_back(newBall);
              }
          }
      }
    }
}

bool BallWindow::hasBallWithId(quint32 id) const {
    for (const auto& ball : m_balls) {
        if (ball.id() == id) {
            return true;
        }
    }
    return false;
}

void BallWindow::removeBallWithId(int targetId)
{
    m_balls.erase(std::remove_if(m_balls.begin(), m_balls.end(),
                                 [targetId](const Ball& ball) { return ball.id() == targetId; }),
                  m_balls.end());
}

bool BallWindow::isBallInRightWindow(const Ball &ball,const QRect& thisRect, const QVector<WindowInfo> &otherWindows)
{
    bool result = false;
    for (const WindowInfo& win : otherWindows) {
        QRectF otherRect(win.x, win.y, win.width, win.height);
        result = otherRect.contains(thisRect.x() + ball.position().x() + ball.radius(), thisRect.y() + ball.position().y());
        if(result)
        {
            break;
        }
    }
    return result;
}
bool BallWindow::isBallInLeftWindow(const Ball &ball,const QRect& thisRect, const QVector<WindowInfo> &otherWindows)
{
    bool result = false;
    for (const WindowInfo& win : otherWindows) {
        QRectF otherRect(win.x, win.y, win.width, win.height);
        result = otherRect.contains(thisRect.x() + ball.position().x() - ball.radius(), thisRect.y() + ball.position().y());
        if(result)
        {
            break;
        }
    }
    return result;
}
bool BallWindow::isBallInTopWindow(const Ball &ball,const QRect& thisRect, const QVector<WindowInfo> &otherWindows)
{
    bool result = false;
    for (const WindowInfo& win : otherWindows) {
        QRectF otherRect(win.x, win.y, win.width, win.height);
        result = otherRect.contains(thisRect.x() + ball.position().x(), thisRect.y() + ball.position().y() - ball.radius());
        if(result)
        {
            break;
        }
    }
    return result;
}
bool BallWindow::isBallInBottomWindow(const Ball &ball,const QRect& thisRect, const QVector<WindowInfo> &otherWindows)
{
    bool result = false;
    for (const WindowInfo& win : otherWindows) {
        QRectF otherRect(win.x, win.y, win.width, win.height);
        result = otherRect.contains(thisRect.x() + ball.position().x(), thisRect.y() + ball.position().y() + ball.radius());
        if(result)
        {
            break;
        }
    }
    return result;
}
