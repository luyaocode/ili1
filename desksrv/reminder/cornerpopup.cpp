#include "cornerpopup.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QScreen>
#include <QGuiApplication>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>

CornerPopup::CornerPopup(const QString &text, QWidget *parent): QWidget(parent)
{
    // 窗口设置：无边框、置顶、半透明背景
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_DeleteOnClose);  // 关闭时自动销毁

    // 设置弹窗样式：白色背景、圆角、阴影
    setStyleSheet(R"(
                  QWidget {
                  background-color: green;
                  border-radius: 8px;
                  }
                  )");

    // 添加阴影效果
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setColor(QColor(0, 0, 0, 80));
    shadow->setBlurRadius(10);
    shadow->setOffset(2, 2);
    setGraphicsEffect(shadow);

    // 文本标签
    QLabel *textLabel = new QLabel(text, this);
    textLabel->setStyleSheet("color: #ffffff; font-size: 14px; padding: 14px;");
    textLabel->setWordWrap(true);     // 自动换行
    textLabel->setMaximumWidth(800);  // 限制最大宽度

    // 布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(textLabel);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    // 定位到右下角
    moveToBottomRight();

    // 3秒后自动关闭（带淡出动画）
    QTimer::singleShot(3000, this, [this]() {
        if (this->isVisible())
        {
            // 淡出动画
            QPropertyAnimation *anim = new QPropertyAnimation(this, "windowOpacity");
            anim->setDuration(500);
            anim->setStartValue(1.0);
            anim->setEndValue(0.0);
            // 动画结束后：先关闭弹窗，再退出应用
            connect(anim, &QPropertyAnimation::finished, this, [this]() {
                this->close();  // 关闭弹窗
                qApp->quit();   // 退出整个应用程序
            });
            anim->start(QAbstractAnimation::DeleteWhenStopped);
        }
    });

    // 显示时淡入动画
    setWindowOpacity(0.0);
    QPropertyAnimation *showAnim = new QPropertyAnimation(this, "windowOpacity");
    showAnim->setDuration(500);
    showAnim->setStartValue(0.0);
    showAnim->setEndValue(1.0);
    showAnim->start(QAbstractAnimation::DeleteWhenStopped);
    adjustSize();  // 调整高度以适应内容（宽度已固定）
}

void CornerPopup::moveToBottomRight()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;

    QRect screenRect = screen->geometry();  // 使用整个屏幕区域，包括任务栏

    // 计算位置
    int x = screenRect.right();
    int y = screenRect.bottom();

    // 对于某些窗口管理器，可能需要使用窗口标志
    this->setWindowFlags(this->windowFlags() | Qt::WindowStaysOnBottomHint);
    this->move(x, y);

    // 立即刷新窗口位置
    this->show();
    this->activateWindow();
}
