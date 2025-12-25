#include "buttontoolbar.h"

#include <QFrame>
#include <QSplitter>
#include <QDebug>

ButtonToolBar::ButtonToolBar(QWidget *parent) : QWidget(parent)
{

}

void ButtonToolBar::initControls()
{
    // 初始化水平布局，按钮将水平排列
    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(5, 2, 5, 2); // 工具栏边缘间距
    m_layout->setSpacing(4); // 按钮之间的间距
    // 注意：需在添加按钮之前设置，确保伸缩项在所有按钮的右边
    m_layout->addStretch();
    // 设置工具栏样式（可选，根据需要调整）
    setStyleSheet(R"(
          QPushButton {
              border: none; /* 无边框 */
              color: black; /* 文本黑色 */
              padding: 2px 5px; /* 适当添加内边距，让按钮更美观 */
              font-size: 10px;
          }
          QPushButton:hover {
              background-color: gray; /* 鼠标悬浮时底色灰色 */
              color: white; /* 鼠标悬浮时文本白色 */
          }
      )");
    setFixedHeight(32);
}

QPushButton *ButtonToolBar::addButton(const QString &buttonName, ButtonToolBar::ButtonCallback callback, const QString& toolTip, const QIcon &icon) {
    if(!m_layout || layout()->count() == 0)
    {
        qDebug()<<"[ButtonToolBar] 请先执行initControls()!";
        Q_ASSERT(false);
    }
    // 创建按钮并设置属性
    QPushButton* btn = new QPushButton(buttonName, this);
    btn->setFixedHeight(32);
    if(!toolTip.isEmpty())
    {
        btn->setToolTip(toolTip);
    }
    if (!icon.isNull()) {
        btn->setIcon(icon);
        // 可选：设置图标文本排列方式
        btn->setIconSize(QSize(16, 16));
    }

    // 绑定回调函数
    connect(btn, &QPushButton::clicked, this, [=]() {
        if (callback) {
            callback(); // 执行回调
        }
    });

    // 添加到布局
    m_layout->insertWidget(m_layout->count()-1,btn);
    return btn;
}

void ButtonToolBar::addSeparator(int index) {
    if(!m_layout || layout()->count() == 0)
    {
        qDebug()<<"[ButtonToolBar] 请先执行initControls()!";
        Q_ASSERT(false);
    }
    if(index < -1 || index > m_layout->count()-1)
    {
        return;
    }
    if(index == -1)
    {
        index = m_layout->count()-1;
    }
    QFrame* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine); // 垂直分隔线
    separator->setFrameShadow(QFrame::Sunken);
    separator->setFixedHeight(20); // 分隔线高度
    m_layout->insertWidget(index, separator);
}

void ButtonToolBar::clear() {
    // 移除并删除所有子部件
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
}

TopButtonToolBar::TopButtonToolBar(QWidget *parent):ButtonToolBar(parent)
{
    setObjectName("topButtonToolBar");
    setAttribute(Qt::WA_StyledBackground, true);
}

void TopButtonToolBar::initControls()
{
    // 初始化水平布局，按钮将水平排列
    m_layout = new QHBoxLayout;
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 10, 0); // 工具栏边缘间距
    // 初始化水平布局，按钮将水平排列
    m_layout->setContentsMargins(5, 5, 5, 5); // 工具栏边缘间距
    m_layout->setSpacing(4); // 按钮之间的间距
    // 直接给当前工具栏（this）设置样式，background-color 就是背景色
    setStyleSheet(R"(
        #topButtonToolBar
        {
            background-color: #ffffff;
            border-bottom: 1px solid #cccccc;
        }
        QPushButton
        {
            border: none; /* 无边框 */
            color: black; /* 文本黑色 */
            padding: 5px 10px; /* 适当添加内边距，让按钮更美观 */
        }
        QPushButton:hover
        {
            background-color: gray; /* 鼠标悬浮时底色灰色 */
            color: white; /* 鼠标悬浮时文本白色 */
        }
    )");
    m_layout->addStretch();
    setFixedHeight(40);
    auto layoutRight = new QHBoxLayout;
    QPushButton* btn = new QPushButton("Exit", this);
    btn->setStyleSheet(R"(
                       QPushButton {
                           border: none;
                           color: red;
                           padding: 5px 10px;
                       }
                       QPushButton:hover {
                           background-color: red; /* 鼠标悬浮时底色灰色 */
                           color: white;
                       }
                       )");
    connect(btn, &QPushButton::clicked, this, &TopButtonToolBar::sigExit);
    btn->setFixedHeight(32);
    layoutRight->addWidget(btn);
    layout->addLayout(m_layout);
    layout->addLayout(layoutRight);
}
