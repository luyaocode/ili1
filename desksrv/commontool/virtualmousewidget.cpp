#include "virtualmousewidget.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
VirtualMouseWidget::VirtualMouseWidget(QWidget *parent) : QWidget(parent)
{
    // 窗口属性：无边框、置顶、透明、事件穿透
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                   Qt::Tool | Qt::X11BypassWindowManagerHint);
    setAttribute(Qt::WA_TranslucentBackground); // 透明背景
    setAttribute(Qt::WA_TransparentForMouseEvents); // 事件穿透
    setCursor(Qt::BlankCursor); // 隐藏原生鼠标（可选）

    // 窗口覆盖整个屏幕
    QScreen *screen = QApplication::primaryScreen();
    QRect screenRect = screen->geometry();
    setGeometry(screenRect);

    // 初始化鼠标坐标
    m_mouseX = screenRect.width() / 2;
    m_mouseY = screenRect.height() / 2;
    m_mouseSize = 24; // 鼠标指针尺寸
    m_isPressed = false;

    show();
}

void VirtualMouseWidget::updateMousePos(int x, int y)
{
    m_mouseX = x;
    m_mouseY = y;
    update(); // 触发重绘
}

void VirtualMouseWidget::updateMousePressState(bool pressed)
{
    m_isPressed = pressed;
    update();
}

void VirtualMouseWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // 抗锯齿

    // 绘制自定义鼠标指针（箭头样式，高辨识度）
    drawMouseArrow(&painter);
    // 可选：绘制圆形鼠标（简化版）
    // drawMouseCircle(&painter);
}

void VirtualMouseWidget::drawMouseArrow(QPainter *painter)
{
    int arrowSize = m_mouseSize;
    QPainterPath path;

    // 箭头路径（中心点为m_mouseX/m_mouseY）
    path.moveTo(m_mouseX, m_mouseY);
    path.lineTo(m_mouseX + arrowSize, m_mouseY + arrowSize / 2);
    path.lineTo(m_mouseX + arrowSize / 3, m_mouseY + arrowSize);
    path.lineTo(m_mouseX, m_mouseY + arrowSize / 3);
    path.closeSubpath();

    // 按压状态样式变化
    if (m_isPressed) {
        painter->setBrush(QColor(255, 0, 0, 180)); // 按压时半透红
        painter->setPen(QPen(Qt::red, 2));
    } else {
        painter->setBrush(QColor(0, 0, 0, 180));   // 正常时半透黑
        painter->setPen(QPen(Qt::black, 2));
    }

    painter->drawPath(path);

    // 白色描边增强辨识度
    painter->setBrush(Qt::transparent);
    painter->setPen(QPen(Qt::white, 1));
    painter->drawPath(path);
}

void VirtualMouseWidget::drawMouseCircle(QPainter *painter)
{
    QPen pen(m_isPressed ? Qt::red : Qt::black, 2);
    QBrush brush(m_isPressed ? QColor(255, 0, 0, 100) : Qt::transparent);
    painter->setPen(pen);
    painter->setBrush(brush);
    painter->drawEllipse(m_mouseX - m_mouseSize/2, m_mouseY - m_mouseSize/2,
                         m_mouseSize, m_mouseSize);
}
