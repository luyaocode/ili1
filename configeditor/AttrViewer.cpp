#include "AttrViewer.h"
#include <QHeaderView>
#include "richtextdelegate.h"

AttrViewer::AttrViewer(QWidget *parent)
    : QTableWidget(parent)
{
    // 初始化两列布局
    setColumnCount(2);

    // 默认表头
    setHorizontalHeaderLabels({"属性", "值"});  // 修正：原setHeaderLabels可能是自定义函数，这里用标准方法

    // 样式优化
    QHeaderView* horizontalHeader = this->horizontalHeader();

    // 第一列：允许用户调整宽度，最小宽度200px，初始宽度至少200px
    horizontalHeader->setSectionResizeMode(0, QHeaderView::Interactive);  // 允许手动调整
    horizontalHeader->setMinimumSectionSize(200);  // 所有列最小宽度（若仅需第一列，见下方补充）
    horizontalHeader->resizeSection(0, 200);  // 初始宽度设为200px

    // 第二列：拉伸填充剩余空间，同时允许用户调整（优先保证第一列最小宽度）
    horizontalHeader->setSectionResizeMode(1, QHeaderView::Stretch);

    // 其他原有样式
    setEditTriggers(QAbstractItemView::DoubleClicked);  // 禁止编辑
    setSelectionBehavior(QAbstractItemView::SelectRows);  // 整行选择
    setAlternatingRowColors(true);  // 行颜色交替
    setShowGrid(false);  // 隐藏网格线
    setFocusPolicy(Qt::NoFocus);  // 去除焦点框（可选）

    setItemDelegate(new RichTextDelegate(this));
}

// 添加属性键值对
void AttrViewer::addProperty(const QString &key, const QString &value)
{
    const int row = rowCount();
    insertRow(row);
    // 设置单元格内容（确保使用非空item）
    setItem(row, 0, new QTableWidgetItem(key));
    setItem(row, 1, new QTableWidgetItem(value));
}

// 清空所有属性
void AttrViewer::clearProperties()
{
    // 保留表头，删除所有数据行
    while (rowCount() > 0) {
        removeRow(0);
    }
}

// 自定义表头文本
void AttrViewer::setHeaderLabels(const QString &keyLabel, const QString &valueLabel)
{
    setHorizontalHeaderLabels({keyLabel, valueLabel});
}

void AttrViewer::slotData(const QString &key, const QString &value)
{
    // 确保表格有且仅有一行（若没有则创建一行）
    if (rowCount() == 0) {
       insertRow(0); // 若表格为空，插入第一行
    } else if (rowCount() > 1) {
       // 若表格行数超过1行，删除多余行（确保只有一行）
       while (rowCount() > 1) {
           removeRow(1); // 从第二行开始删除
       }
    }

    setRichTextCell(0,0,key);
    setRichTextCell(0,1,value);
}

void AttrViewer::setRichTextCell(int row, int column, const QString &richText) {
    QTableWidgetItem* item = new QTableWidgetItem;
    item->setText(richText);
    setItem(row, column, item);

    // 关键修复：通过QStyle获取视图选项，替代直接调用viewOptions()
    QStyleOptionViewItem option;
    option.initFrom(this); // 从表格初始化选项（包含字体、样式等）
    option.rect.setWidth(columnWidth(column)); // 设置当前列宽

    // 计算并调整行高
    QSize size = itemDelegate()->sizeHint(option, model()->index(row, column));
    setRowHeight(row, size.height());
    // 自动调整列宽以适应新内容
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}
