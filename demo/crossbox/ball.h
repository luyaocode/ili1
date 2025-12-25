#ifndef BALL_H
#define BALL_H
#include "inc.h"

// 小球类定义
class Ball {
public:
    Ball(const QRectF& bounds);

    void update(bool leftConnected, bool rightConnected, bool topConnected, bool bottomConnected , QPointF windowVelocity);

    // 检查是否超出某个边界
    bool isOutOfLeft() const { return m_position.x() + m_radius < m_bounds.left(); }
    bool isOutOfRight() const { return m_position.x() - m_radius > m_bounds.right(); }
    bool isOutOfTop() const { return m_position.y() + m_radius < m_bounds.top(); }
    bool isOutOfBottom() const { return m_position.y() - m_radius > m_bounds.bottom(); }

    // Getter方法
    QPointF position() const { return m_position; }
    float radius() const { return m_radius; }
    float rotation() const { return m_rotation; }
    float rotationSpeed() const {return m_rotationSpeed;}
    QVector2D velocity() const { return m_velocity; }
    quint32 id() const { return m_id; }

    // 设置属性（用于从其他进程接收小球）
    void setId(quint32 id){m_id = id;}
    void setPosition(const QPointF& pos) { m_position = pos; }
    void setVelocity(const QVector2D& vel) { m_velocity = vel; }
    void setRotation(float rot) { m_rotation = rot; }
    void setRotationSpeed(float speed) { m_rotationSpeed = speed; }
private:
    // 检查边界碰撞，根据连接状态决定是否反弹
    void checkBoundsCollision(bool leftConnected, bool rightConnected, bool topConnected, bool bottomConnected , QPointF windowVelocity);
private:
    QRectF m_bounds;          // 边界
    QPointF m_position;       // 位置，相对m_bounds
    QVector2D m_velocity;     // 速度
    float m_radius;           // 半径
    float m_rotation;         // 旋转角度
    float m_rotationSpeed;    // 旋转速度
    quint32 m_id;             // 唯一ID
};

#endif // BALL_H
