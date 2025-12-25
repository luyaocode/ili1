#include "ball.h"

// 移动窗口速度增益
const int WINDOW_VELOCITY_FACTOR = 10;
// 碰撞速度衰减
const float CRASH_VELOCITY_FACTOR = 0.9;
// 重力因子
const float G_VELOCITY_FACTOR = 0.02;

Ball::Ball(const QRectF &bounds) : m_bounds(bounds) {
    // 初始化位置在中心
    m_position = bounds.center();

    // 随机速度
    float speed = 4.0f + (float)qrand() / RAND_MAX * 2.0f;

    // 随机方向
    float angle = (double)qrand() * 2 * M_PI;
    m_velocity = QVector2D(std::cos(angle) * speed, std::sin(angle) * speed);

    // 随机自转速度和方向
    m_rotationSpeed = 10.0f + (double)qrand()/RAND_MAX * 10.0f;
    if ((double)qrand() < 0.5) {
        m_rotationSpeed = -m_rotationSpeed;
    }

    m_radius = 15.0f;
    m_rotation = 0.0f;
    m_id = (quint32)qrand(); // 唯一ID用于进程间识别
}

void Ball::update(bool leftConnected, bool rightConnected, bool topConnected, bool bottomConnected, QPointF windowVelocity) {
    // 更新位置
    m_position += QPointF(m_velocity.x(), m_velocity.y());

    // 更新自转
    m_rotation += m_rotationSpeed;
    while (m_rotation >= 360) m_rotation -= 360;
    while (m_rotation < 0) m_rotation += 360;

    checkBoundsCollision(leftConnected, rightConnected, topConnected, bottomConnected, windowVelocity);
}

void Ball::checkBoundsCollision(bool leftConnected, bool rightConnected, bool topConnected, bool bottomConnected , QPointF windowVelocity)
{
    // 保存碰撞前的速度方向（用于后续判断）
    float preCollisionVelX = m_velocity.x();
    float preCollisionVelY = m_velocity.y();

    // 左边界
    if (m_position.x() - m_radius < m_bounds.left()) {
        if (!leftConnected) {
            // 修正位置，避免小球卡在边界
           m_position.setX(m_bounds.left() + m_radius);
           // 基础反弹：水平速度反向
           m_velocity.setX((-preCollisionVelX)*CRASH_VELOCITY_FACTOR);

           // 结合窗口水平速度调整反弹速度
           if (windowVelocity.x() != 0) {  // 窗口水平方向有移动
               // 小球碰撞前的水平方向（左移：preCollisionVelX < 0，因为撞向左边界时小球正在向左运动）
               int ballDirX = (preCollisionVelX < 0) ? -1 : 1;
               // 窗口水平移动方向（右移为正，左移为负）
               int windowDirX = (windowVelocity.x() > 0) ? 1 : -1;

               // 影响系数：窗口速度绝对值（速度越快影响越大），可根据实际效果调整缩放因子
               float influence = qAbs(windowVelocity.x()) * WINDOW_VELOCITY_FACTOR;

               if (ballDirX == windowDirX) {
                   // 方向相同（如小球左移时窗口也左移）：反弹速度减小
                   m_velocity.setX(m_velocity.x() - ballDirX * influence);
               } else {
                   // 方向相反（如小球左移时窗口右移）：反弹速度增大
                   m_velocity.setX(m_velocity.x() + ballDirX * influence);
               }
           }
        }
    }
    // 右边界
    else if (m_position.x() + m_radius > m_bounds.right()) {
        if (!rightConnected) {
            // 修正位置
           m_position.setX(m_bounds.right() - m_radius);
           // 基础反弹：水平速度反向
           m_velocity.setX((-preCollisionVelX)*CRASH_VELOCITY_FACTOR);

           // 结合窗口水平速度调整反弹速度
           if (windowVelocity.x() != 0) {
               // 小球碰撞前的水平方向（右移：preCollisionVelX > 0，因为撞向右边界时小球正在向右运动）
               int ballDirX = (preCollisionVelX > 0) ? 1 : -1;
               // 窗口水平移动方向
               int windowDirX = (windowVelocity.x() > 0) ? 1 : -1;

               float influence = qAbs(windowVelocity.x()) * WINDOW_VELOCITY_FACTOR;

               if (ballDirX == windowDirX) {
                   // 方向相同（如小球右移时窗口也右移）：反弹速度减小
                   m_velocity.setX(m_velocity.x() - ballDirX * influence);
               } else {
                   // 方向相反（如小球右移时窗口左移）：反弹速度增大
                   m_velocity.setX(m_velocity.x() + ballDirX * influence);
               }
           }
        }
    }
    // 上边界
    if (m_position.y() - m_radius < m_bounds.top()) {
        if (!topConnected) {
            // 修正位置
           m_position.setY(m_bounds.top() + m_radius);
           // 基础反弹：垂直速度反向
           m_velocity.setY((-preCollisionVelY)*CRASH_VELOCITY_FACTOR);

           // 结合窗口垂直速度调整反弹速度
           if (windowVelocity.y() != 0) {  // 窗口垂直方向有移动
               // 小球碰撞前的垂直方向（上移：preCollisionVelY < 0，因为撞上上边界时小球正在向上运动）
               int ballDirY = (preCollisionVelY < 0) ? -1 : 1;
               // 窗口垂直移动方向（下移为正，上移为负）
               int windowDirY = (windowVelocity.y() > 0) ? 1 : -1;

               float influence = qAbs(windowVelocity.y()) * WINDOW_VELOCITY_FACTOR;

               if (ballDirY == windowDirY) {
                   // 方向相同（如小球上移时窗口也上移）：反弹速度减小
                   m_velocity.setY(m_velocity.y() - ballDirY * influence);
               } else {
                   // 方向相反（如小球上移时窗口下移）：反弹速度增大
                   m_velocity.setY(m_velocity.y() + ballDirY * influence);
               }
           }
        }
    }
    // 下边界
    else if (m_position.y() + m_radius > m_bounds.bottom()) {
        if (!bottomConnected) {
            // 修正位置
            m_position.setY(m_bounds.bottom() - m_radius);
            // 基础反弹：垂直速度反向
            m_velocity.setY((-preCollisionVelY)*CRASH_VELOCITY_FACTOR);

            // 结合窗口垂直速度调整反弹速度
            if (windowVelocity.y() != 0) {
                // 小球碰撞前的垂直方向（下移：preCollisionVelY > 0，因为撞上下边界时小球正在向下运动）
                int ballDirY = (preCollisionVelY > 0) ? 1 : -1;
                // 窗口垂直移动方向
                int windowDirY = (windowVelocity.y() > 0) ? 1 : -1;

                float influence = qAbs(windowVelocity.y()) * WINDOW_VELOCITY_FACTOR;

                if (ballDirY == windowDirY) {
                    // 方向相同（如小球下移时窗口也下移）：反弹速度减小
                    m_velocity.setY(m_velocity.y() - ballDirY * influence);
                } else {
                    // 方向相反（如小球下移时窗口上移）：反弹速度增大
                    m_velocity.setY(m_velocity.y() + ballDirY * influence);
                }
            }
        }
    }

}
