#ifndef VIRTUALMOUSEWIDGET_H
#define VIRTUALMOUSEWIDGET_H
#include <QWidget>
// 虚拟鼠标绘制窗口（全局悬浮透明）
class VirtualMouseWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VirtualMouseWidget(QWidget *parent = nullptr);

    // 更新鼠标绘制坐标
    void updateMousePos(int x, int y);

    // 更新鼠标按压状态（用于样式变化）
    void updateMousePressState(bool pressed);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_mouseX;      // 鼠标X坐标
    int m_mouseY;      // 鼠标Y坐标
    int m_mouseSize;   // 鼠标尺寸
    bool m_isPressed;  // 鼠标按压状态

    // 绘制箭头样式鼠标（Linux原生风格）
    void drawMouseArrow(QPainter *painter);

    // 绘制圆形鼠标（简化版）
    void drawMouseCircle(QPainter *painter);
};
#endif // VIRTUALMOUSEWIDGET_H
