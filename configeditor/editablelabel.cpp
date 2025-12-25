#include "editablelabel.h"
#include <QVBoxLayout>
#include <QMouseEvent>

EditableLabel::EditableLabel(QWidget *parent)
    : QLabel(parent), m_lineEdit(nullptr)
{
    init();
}

EditableLabel::EditableLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent), m_lineEdit(nullptr)
{
    init();
}

void EditableLabel::init()
{
    // 设置文本对齐方式
    setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    // 允许文本换行
    setWordWrap(false);

    // 创建用于编辑的LineEdit
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setVisible(false);
    m_lineEdit->setFont(font());

    // 连接编辑完成信号
    connect(m_lineEdit, &QLineEdit::editingFinished,
            this, &EditableLabel::onEditingFinished);
}

void EditableLabel::mouseDoubleClickEvent(QMouseEvent *event)
{
    // 双击时显示输入框并设置焦点
    if (event->button() == Qt::LeftButton) {
        m_lineEdit->setText(text());
        m_lineEdit->setGeometry(0, 0, width(), height());
        m_lineEdit->setVisible(true);
        m_lineEdit->setFocus();
    }
    QLabel::mouseDoubleClickEvent(event);
}

void EditableLabel::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    // 调整输入框大小以匹配标签
    m_lineEdit->setGeometry(0, 0, width(), height());
}

QSize EditableLabel::sizeHint() const
{
    // 计算合适的大小
    QFontMetrics fm(font());
    int textWidth = fm.width(text());
    int textHeight = fm.height();

    // 获取屏幕可用宽度
    QScreen *screen = QApplication::primaryScreen();
    if (!screen) return QLabel::sizeHint();

    QRect screenGeometry = screen->availableGeometry();
    int maxWidth = screenGeometry.width();

    // 确保文本宽度不超过屏幕宽度
    int desiredWidth = qMin(textWidth, maxWidth);

    return QSize(desiredWidth, textHeight);
}

QSize EditableLabel::minimumSizeHint() const
{
    // 最小大小为文本高度和最小宽度
    QFontMetrics fm(font());
    return QSize(10, fm.height());
}

void EditableLabel::mousePressEvent(QMouseEvent *event)
{
    // 转发点击事件（左键点击有效）
    if (event->button() == Qt::LeftButton)
    {
       emit labelClicked();
    }
    QLabel::mousePressEvent(event); // 保留原有行为
}

void EditableLabel::onEditingFinished()
{
    // 编辑完成后更新文本并隐藏输入框
    setText(m_lineEdit->text());
    m_lineEdit->setVisible(false);
    // 触发大小调整以适应新文本
    updateGeometry();
}
