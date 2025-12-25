#include "NotifyPopup.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QScreen>
#include <QPalette>
#include <QFont>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QEvent>

NotifyPopup::NotifyPopup(const QString &title, const QString &message, QWidget *parent): QWidget(parent)
{
    // 1. 窗口基础设置：无边框、置顶、扁平化
    setWindowFlags(Qt::FramelessWindowHint |   // 去掉窗口边框
                   Qt::WindowStaysOnTopHint |  // 始终置顶
                   Qt::Tool);                  // 工具窗口，不占任务栏空间

    setAttribute(Qt::WA_DeleteOnClose);          // 关闭时自动释放内存
    setAttribute(Qt::WA_TranslucentBackground);  // 透明背景（可选，用于纯扁平化）

    // 2. 标题栏容器：用于放置标题文本和叉号关闭按钮
    QWidget *titleBarWidget = new QWidget(this);
    titleBarWidget->setStyleSheet(R"(
        QWidget {
            background-color: #2196F3; /* 与原标题栏背景一致 */
            /* 移除：标题栏的边框属性 */
            /* border: 1px solid #0D47A1; */
        }
    )");
    titleBarWidget->setFixedHeight(32);  // 与原标题栏高度一致

    // 2.1 标题文本标签
    QLabel *titleLabel = new QLabel(title, titleBarWidget);
    titleLabel->setStyleSheet(R"(
        QLabel {
            background-color: transparent; /* 透明背景，继承父容器的背景 */
            color: #FFFFFF; /* 白色字体 */
            padding: 8px 10px; /* 内边距保持不变 */
            font-size: 12px; /* 字体改小：从14px改为12px */
            font-weight: normal; /* 扁平化取消粗体 */
            line-height: 12px; /* 行高同步改小，匹配字体大小 */
        }
    )");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);  // 左对齐+垂直居中

    // 2.2 叉号关闭标签（模拟按钮）
    QLabel *closeLabel = new QLabel("×", titleBarWidget);
    closeLabel->setStyleSheet(R"(
        QLabel {
            background-color: transparent; /* 透明背景 */
            color: #FFFFFF; /* 白色叉号 */
            font-size: 14px; /* 叉号字体同步改小：从16px改为14px，与标题比例协调 */
            padding: 0 10px; /* 左右内边距，扩大点击区域 */
            border: none; /* 无边框 */
        }
        QLabel:hover {
            color: #f0f0f0; /* hover时叉号变浅，提升交互感 */
            background-color: rgba(0, 0, 0, 0.1); /* 轻微背景色，提示可点击 */
        }
    )");
    closeLabel->setAlignment(Qt::AlignCenter);      // 叉号居中显示
    closeLabel->setCursor(Qt::PointingHandCursor);  // 鼠标悬浮时显示手型，提示可点击
    closeLabel->installEventFilter(this);

    // 2.3 标题栏的水平布局：标题左对齐，叉号右对齐
    QHBoxLayout *titleLayout = new QHBoxLayout(titleBarWidget);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();  // 拉伸项，将叉号推到右侧
    titleLayout->addWidget(closeLabel);
    titleLayout->setSpacing(0);
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleBarWidget->setLayout(titleLayout);

    // 3. 创建正文标签
    QLabel *msgLabel = new QLabel(message, this);
    // 正文样式：调整边框，去掉border-top: none，因为标题栏已无边框，避免衔接断层
    msgLabel->setStyleSheet(R"(
          QLabel {
              background-color: #f5f5f5; /* 浅灰色背景（扁平化常用） */
              color: #2196F3; /* 蓝色文本（与标题栏蓝色一致） */
              padding: 12px 12px; /* 内边距保持不变 */
              font-size: 13px;
              border: 1px solid #dddddd; /* 像素风边框：1px浅灰边框 */
              /* 移除：border-top: none，因为标题栏已无边框 */
              /* border-top: none; */
              /* 可选：极致像素风用标题栏同款深海军蓝边框 */
              /* border: 1px solid #0D47A1; */
          }
      )");
    msgLabel->setWordWrap(true);                           // 自动换行（必须保留，避免长文本溢出）
    msgLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);  // 左上对齐

    // 4. 主布局：垂直排列标题栏和正文，无间距、无边距（贴边）
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(titleBarWidget);
    mainLayout->addWidget(msgLabel);
    mainLayout->setSpacing(0);                   // 标题栏和正文之间无间距
    mainLayout->setContentsMargins(0, 0, 0, 0);  // 布局无内边距（贴边）
    setLayout(mainLayout);

    // 5. 3秒后自动关闭（注释保留，可根据需要开启）
    //    QTimer::singleShot(3000, this, &NotifyPopup::close);
}

// 事件过滤器，处理叉号标签的点击事件
bool NotifyPopup::eventFilter(QObject *watched, QEvent *event)
{
    // 判断是否是叉号标签的鼠标左键点击事件
    if (watched->inherits("QLabel") && event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            close();      // 点击后关闭弹窗
            return true;  // 事件已处理
        }
    }
    return QWidget::eventFilter(watched, event);  // 其他事件交给父类处理
}
