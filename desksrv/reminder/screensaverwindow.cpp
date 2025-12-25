#include "screensaverwindow.h"
#include <QDebug>
#include <X11/keysym.h>

#include "commontool.h"
#include "keyblocker.h"

ScreenSaverWindow::ScreenSaverWindow(const QString &imagePath, QWidget *parent)
    : QMainWindow(parent), m_imagePath(imagePath), m_angle(0)
{
    // 设置窗口属性
    setWindowTitle("屏幕保护");
    setWindowFlags(Qt::FramelessWindowHint);      // 无边框
    Commontool::setOverrideRedirect(this, true);  // 设置禁用切换窗口

    // 检查图片是否存在
    m_hasImage = m_pixmap.load(m_imagePath);

    // 如果没有图片，设置定时器用于屏幕保护动画
    if (!m_hasImage)
    {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &ScreenSaverWindow::updateAnimation);
        m_timer->start(50);  // 每50毫秒更新一次动画
    }

    // 启动拦截Win
    m_winBlocker.reset(new KeyBlocker({KeyWithModifier(XK_Super_L),KeyWithModifier(XK_Super_R)}));
    connect(m_winBlocker.data(), &KeyBlocker::sigStarted, []() {
        qDebug() << "[ScreenSaverWindow] Win 键拦截已启动";
    });
    connect(m_winBlocker.data(), &KeyBlocker::sigStopped, []() {
        qDebug() << "[ScreenSaverWindow] Win 键拦截已停止";
    });
    connect(m_winBlocker.data(), &KeyBlocker::sigErrorOccurred, [](const QString &msg) {
        qWarning() << "[ScreenSaverWindow] 拦截错误：" << msg;
    });
    connect(m_winBlocker.data(), &KeyBlocker::sigBlocked, this, []() {});
    m_winBlocker->start();

    show();
}

ScreenSaverWindow::~ScreenSaverWindow()
{
}

void ScreenSaverWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_hasImage)
    {
        // 绘制图片，保持比例并居中显示
        QRect targetRect = rect();
        QRect sourceRect = m_pixmap.rect();

        // 计算缩放比例
        qreal scale =
            qMin((qreal)targetRect.width() / sourceRect.width(), (qreal)targetRect.height() / sourceRect.height());

        int scaledWidth  = sourceRect.width() * scale;
        int scaledHeight = sourceRect.height() * scale;

        // 计算居中位置
        int x = (targetRect.width() - scaledWidth) / 2;
        int y = (targetRect.height() - scaledHeight) / 2;

        painter.drawPixmap(x, y, scaledWidth, scaledHeight, m_pixmap);
    }
    else
    {
        // 绘制屏幕保护效果
        painter.fillRect(rect(), Qt::black);  // 黑色背景

        // 绘制旋转的彩色矩形
        int centerX = width() / 2;
        int centerY = height() / 2;

        painter.save();
        painter.translate(centerX, centerY);
        painter.rotate(m_angle);

        // 绘制多个矩形
        QColor colors[] = {
            QColor(144, 238, 144),  // 浅绿
            QColor(152, 251, 152),  // 苍白绿
            QColor(124, 252, 0),    //  lawn绿
            QColor(34, 139, 34),    // 森林绿
            QColor(46, 139, 87),    // 海绿
            QColor(60, 179, 113)    //  MediumSpringGreen
        };
        int rectCount = 6;
        int baseSize  = qMin(width(), height()) / 4;

        for (int i = 0; i < rectCount; ++i)
        {
            int size = baseSize * (i + 1) / rectCount;
            painter.setBrush(colors[i % 6]);
            painter.setPen(Qt::NoPen);
            painter.drawRect(-size / 2, -size / 2, size, size);
        }

        painter.restore();

        // 绘制当前时间
        QDateTime currentTime = QDateTime::currentDateTime();
        QString   timeStr     = currentTime.toString("yyyy-MM-dd HH:mm:ss");
        painter.setPen(Qt::lightGray);
        QFont font = painter.font();
        font.setPointSize(24);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignBottom | Qt::AlignLeft, timeStr);

        // 绘制提示语句
        QString closeStr = "Double Click or Press Ctrl+C to Close.";
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter | Qt::AlignCenter, closeStr);
    }
}

// 处理鼠标双击事件
void ScreenSaverWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    close();  // 双击时关闭窗口
}

// 处理键盘事件（仅响应Ctrl+C）
void ScreenSaverWindow::keyPressEvent(QKeyEvent *event)
{
    // 检查是否同时按下了Ctrl键和C键
    if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_C)
    {
        close();  // Ctrl+C时关闭窗口
    }
}

void ScreenSaverWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 获取当前屏幕的几何尺寸（多屏环境下可指定屏幕）
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    setGeometry(screenRect);  // 强制窗口大小为屏幕尺寸
}

void ScreenSaverWindow::closeEvent(QCloseEvent *event)
{
    event->accept();  // 允许窗口关闭
}

void ScreenSaverWindow::updateAnimation()
{
    m_angle = (m_angle + 1) % 360;
    update();  // 触发重绘
}
