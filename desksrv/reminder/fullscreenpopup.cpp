#include "fullscreenpopup.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QVBoxLayout>
#include <QStyle>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QLabel>
#include <QScreen>
#include <QTimer>
#include "commontool.h"

FullscreenPopup::FullscreenPopup(QWidget *parent): QDialog(parent)
{
    // 1. 窗口基础设置（确保全屏无边界）
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground, false);  // 允许背景绘制

    // 关键修改：获取所有屏幕的联合区域（覆盖多屏）
    QRect fullScreenRect;
    for (const QScreen *screen : QGuiApplication::screens())
    {
        fullScreenRect = fullScreenRect.united(screen->geometry());
    }
    setGeometry(fullScreenRect);  // 设置为所有屏幕的联合区域
    setMinimumSize(fullScreenRect.size());

    // 2. 主容器（覆盖全屏，去除圆角避免边缘空白）
    QWidget *mainContainer = new QWidget(this);
    mainContainer->setStyleSheet(R"(
        QWidget {
            background-color: rgba(30, 30, 30, 0.92); /* 深灰半透明 */
            /* 移除圆角，确保边缘无空白 */
        }
    )");
    mainContainer->setGeometry(fullScreenRect);  // 容器也强制全屏

    // 3. 关闭按钮（保持样式）
    QPushButton *closeBtn = new QPushButton("关闭弹窗", mainContainer);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3a7bd5;
            color: white;
            font-size: 20px;
            font-weight: 600;
            padding: 10px 20px;
            border-radius: 8px;
            border: 1px solid #2d6bc5;
        }
        QPushButton:hover {
            background-color: #ed3f15;
            border: 1px solid #3a7bd5;
            color: black;
        }
        QPushButton:pressed {
            background-color: #2d6bc5;
            border: 1px solid #1d5bb5;
        }
    )");
    closeBtn->setFixedSize(150, 60);
    closeBtn->setCursor(Qt::PointingHandCursor);

    // 在合适的地方（如构造函数）创建定时器并连接信号槽
    QTimer* timer = new QTimer(this); // 确保父对象正确，避免内存泄漏

    // 定义位置随机化的lambda函数
    auto setRandomPosition = [=]() {
        // 容器可用区域（x范围：0 ~ 容器宽度 - 按钮宽度）
        int maxX = mainContainer->width() - closeBtn->width();
        // y范围：0 ~ 容器高度 - 按钮高度
        int maxY = mainContainer->height() - closeBtn->height();

        // 确保边界有效（避免容器尺寸小于按钮时出现负数）
        if (maxX < 0) maxX = 0;
        if (maxY < 0) maxY = 0;

        // 生成0~maxX和0~maxY之间的随机数
        int randomX = qrand() % (maxX + 1);  // +1确保能取到maxX
        int randomY = qrand() % (maxY + 1);

        // 设置按钮位置
        closeBtn->move(randomX, randomY);
    };

    // 4. 随机位置按钮
    connect(timer, &QTimer::timeout, setRandomPosition);
    timer->start(1000);
    setRandomPosition();

    // 5. 主窗口布局（直接填充）
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(mainContainer);
    layout->setContentsMargins(0, 0, 0, 0);  // 关键：去除窗口边缘空白
    setLayout(layout);

    // 6. 按钮点击动画
    connect(closeBtn, &QPushButton::pressed, this, [closeBtn]() {
        closeBtn->move(closeBtn->x(), closeBtn->y() + 2);
    });
    connect(closeBtn, &QPushButton::released, this, [closeBtn]() {
        closeBtn->move(closeBtn->x(), closeBtn->y() - 2);
    });

    // 7. 关闭时淡出动画
    connect(closeBtn, &QPushButton::clicked, this, [this]() {
        QPropertyAnimation *animation = new QPropertyAnimation(this, "windowOpacity");
        animation->setDuration(300);
        animation->setStartValue(1.0);
        animation->setEndValue(0.0);
        connect(animation, &QPropertyAnimation::finished, this, &FullscreenPopup::close);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    });

    connect(closeBtn, &QPushButton::clicked, this, &FullscreenPopup::onCloseClicked);

    // 8. 显示时淡入动画
    setWindowOpacity(0.0);
    QPropertyAnimation *showAnimation = new QPropertyAnimation(this, "windowOpacity");
    showAnimation->setDuration(300);
    showAnimation->setStartValue(0.0);
    showAnimation->setEndValue(1.0);
    showAnimation->start(QAbstractAnimation::DeleteWhenStopped);

    Commontool::setOverrideRedirect(this, true);
}

void FullscreenPopup::onCloseClicked()
{
    close();       // 关闭弹窗
    qApp->quit();  // 退出程序
}

void FullscreenPopup::keyPressEvent(QKeyEvent *event)
{
    // 检查是否同时按下了Ctrl键和C键
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_C)
    {
        close();  // Ctrl+C时关闭窗口
    }
}
