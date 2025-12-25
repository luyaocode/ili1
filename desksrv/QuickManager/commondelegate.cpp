#include "commondelegate.h"
#include <QPainter>
#include <QPushButton>
#include <QApplication>
#include "commonmodel.h"
#include "quickitem.h"

CommonDelegate::CommonDelegate(QObject *parent): QStyledItemDelegate(parent)
{
}

// 封装函数：手绘无边框按钮（悬浮绿色 + 无框 + 文字居中）
void CommonDelegate::drawCustomButton(QPainter                   *painter,
                                      const QStyleOptionViewItem &option,
                                      const QRect                &btnRect,
                                      const QString              &btnText) const
{
    painter->save();

    // 1. 绘制按钮背景
    painter->setPen(Qt::SolidLine);

    painter->setBrush(Qt::transparent);
    // 绘制圆角矩形（无圆角则替换为 painter->drawRect(btnRect)）
    painter->drawRoundedRect(btnRect, 4, 4);

    // 2. 绘制按钮文字（居中）
    QFont btnFont = option.font;
    btnFont.setPointSize(10);  // 字体大小可按需调整
    painter->setFont(btnFont);
    // 文字颜色：悬浮/选中时白色，默认黑色
    if (option.state & QStyle::State_Selected)
    {
        painter->setPen(Qt::white);
    }
    else
    {
        painter->setPen(option.palette.buttonText().color());
    }
    painter->drawText(btnRect, Qt::AlignCenter, btnText);

    painter->restore();
}

void CommonDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // 仅处理操作列
    if (index.column() != static_cast<int>(CommonModel::OperationColunm))
    {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    // 绘制单元格背景
    painter->save();
    if (option.state & QStyle::State_Selected)
    {
        painter->fillRect(option.rect, option.palette.highlight());
    }
    else
    {
        painter->fillRect(option.rect, option.palette.base());
    }
    painter->restore();

    // 计算按钮区域
    QRect terminalRect = getBtnRect(option, TerminalBtn);
    QRect dirRect      = getBtnRect(option, DirectoryBtn);

    // 调用封装函数绘制按钮
    drawCustomButton(painter, option, terminalRect, "打开终端");
    drawCustomButton(painter, option, dirRect, "打开目录");
}

QSize CommonDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    // 操作列增大宽度，保证两个按钮显示完整
    if (index.column() == static_cast<int>(CommonModel::OperationColunm))
        return QSize(200, option.rect.height());  // 宽度200，高度自适应
    return QStyledItemDelegate::sizeHint(option, index);
}

bool CommonDelegate::editorEvent(QEvent                     *event,
                                 QAbstractItemModel         *model,
                                 const QStyleOptionViewItem &option,
                                 const QModelIndex          &index)
{
    if (index.column() != static_cast<int>(CommonModel::OperationColunm))
        return QStyledItemDelegate::editorEvent(event, model, option, index);

    // 仅处理鼠标左键点击
    if (event->type() == QEvent::MouseButtonRelease && static_cast<QMouseEvent *>(event)->button() == Qt::LeftButton)
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QPoint       pos        = mouseEvent->pos();

        // 判断点击的是哪个按钮
        QRect terminalRect = getBtnRect(option, TerminalBtn);
        QRect dirRect      = getBtnRect(option, DirectoryBtn);

        if (terminalRect.contains(pos))
        {
            emit btnClicked(index.row(), TerminalBtn);
            return true;
        }
        else if (dirRect.contains(pos))
        {
            emit btnClicked(index.row(), DirectoryBtn);
            return true;
        }
    }

    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void CommonDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    // 先调用父类方法，获取默认样式
    QStyledItemDelegate::initStyleOption(option, index);

    // 判断当前列是否需要居左对齐
    if (index.column() == CommonModel::Columns::NameColumn || index.column() == CommonModel::Columns::PathColumn ||
            index.column() == CommonModel::Columns::ShortcutColumn)
    {
        // 设置文本居左对齐
        option->displayAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    }
    else
    {
        // 其他列保持居中对齐（可根据需求修改）
        option->displayAlignment = Qt::AlignCenter;
    }
}

QRect CommonDelegate::getBtnRect(const QStyleOptionViewItem &option, ButtonType type) const
{
    int btnWidth  = option.rect.width() / 2 - 10;  // 每个按钮占一半宽度，留10px间距
    int btnHeight = 20;
    int btnY      = option.rect.center().y() - btnHeight / 2;

    if (type == TerminalBtn)
    {
        // 左侧按钮：x从10px开始
        return QRect(option.rect.x() + 10, btnY, btnWidth, btnHeight);
    }
    else
    {
        // 右侧按钮：x从中间+5px开始
        return QRect(option.rect.center().x() + 5, btnY, btnWidth, btnHeight);
    }
}
