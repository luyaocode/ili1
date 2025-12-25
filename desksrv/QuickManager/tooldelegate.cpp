#include "tooldelegate.h"
#include <QPainter>
#include <QApplication>
#include <QMouseEvent>

#include "toolmodel.h"
ToolDelegate::ToolDelegate(QObject *parent): QStyledItemDelegate(parent)
{
}

void ToolDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == ToolModel::StatusColumn)
    {
        bool isValid = index.data(Qt::UserRole).toBool();
        if (isValid)
        {
            // 有效状态，使用默认绘制
            QStyledItemDelegate::paint(painter, option, index);
        }
        else
        {
            // 无效状态，绘制修复按钮
            drawRepairButton(painter, option.rect, option);
        }
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize ToolDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    // 增加行高以适应按钮
    size.setHeight(qMax(size.height(), 30));
    return size;
}

QWidget *ToolDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == ToolModel::ShortcutColumn)
    {
        // 快捷键列使用单行编辑框
        QLineEdit *editor = new QLineEdit(parent);
        editor->setPlaceholderText(tr("例如：Ctrl+Alt+T"));
        return editor;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void ToolDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
    if (index.column() == ToolModel::ShortcutColumn)
    {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
        if (lineEdit)
        {
            lineEdit->setText(index.data(Qt::EditRole).toString());
        }
    }
    else
    {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void ToolDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    if (index.column() == ToolModel::ShortcutColumn)
    {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(editor);
        if (lineEdit)
        {
            model->setData(index, lineEdit->text().trimmed(), Qt::EditRole);
        }
    }
    else
    {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

bool ToolDelegate::editorEvent(QEvent                     *event,
                               QAbstractItemModel         *model,
                               const QStyleOptionViewItem &option,
                               const QModelIndex          &index)
{
    // 1. 先判断是否是目标列（状态列）和目标事件（鼠标释放）
    if (index.column() == ToolModel::StatusColumn && event->type() == QEvent::MouseButtonRelease)
    {
        // ========== 新增：判断当前工具是否需要显示修复按钮（状态无效） ==========
        bool isValid = index.data(Qt::UserRole).toBool();
        if (isValid)
        {
            // 工具状态有效，无修复按钮，直接交给父类处理，不触发任何自定义逻辑
            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }

        // 2. 只有工具状态无效（有修复按钮）时，才校验点击位置
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (isRepairButtonClicked(option.rect, mouseEvent->pos()))
        {
            // 点击修复按钮，执行修复逻辑
            QString newPath =
                QFileDialog::getOpenFileName(nullptr, tr("选择可执行文件"), QDir::homePath(), tr("所有文件 (*)"));

            if (!newPath.isEmpty())
            {
                ToolModel *toolModel = qobject_cast<ToolModel *>(model);
                if (toolModel)
                {
                    toolModel->repairToolPath(index.row(), newPath);
                }
            }
            return true;  // 消耗事件，不传递给父类
        }
        else
        {
            // 点击状态列空白区域（但有修复按钮），交给父类处理
            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }
    }
    // 非状态列或非鼠标释放事件，交给父类处理
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

void ToolDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    // 先调用父类方法，获取默认样式
    QStyledItemDelegate::initStyleOption(option, index);

    // 判断当前列是否需要居左对齐
    if (index.column() == ToolModel::Columns::NameColumn || index.column() == ToolModel::Columns::PathColumn ||
        index.column() == ToolModel::Columns::ShortcutColumn)
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

void ToolDelegate::drawRepairButton(QPainter *painter, const QRect &rect, const QStyleOptionViewItem &option) const
{
    QStyleOptionButton btnOption;
    btnOption.text  = tr("修复");
    btnOption.rect  = rect.adjusted(rect.width() / 4, 2, -rect.width() / 4, -2);  // 居中显示按钮
    btnOption.state = QStyle::State_Enabled;

    // 修复：正确判断鼠标是否悬停在按钮上（坐标转换逻辑简化）
    if (option.widget)
    {
        QPoint mousePos = option.widget->mapFromGlobal(QCursor::pos());  // 鼠标全局坐标 -> 控件局部坐标
        if (btnOption.rect.contains(mousePos))
        {
            btnOption.state |= QStyle::State_MouseOver;  // 悬停时高亮
        }
    }

    QApplication::style()->drawControl(QStyle::CE_PushButton, &btnOption, painter);
}

bool ToolDelegate::isRepairButtonClicked(const QRect &rect, const QPoint &pos) const
{
    QRect btnRect = rect.adjusted(rect.width() / 4, 2, -rect.width() / 4, -2);
    return btnRect.contains(pos);
}
