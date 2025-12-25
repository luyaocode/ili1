#ifndef TOOLDELEGATE_H
#define TOOLDELEGATE_H

#include <QStyledItemDelegate>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>

class ToolDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ToolDelegate(QObject* parent = nullptr);

    // 重写委托接口
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
    // 重写初始化样式选项的方法
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

private:
    // 绘制修复按钮
    void drawRepairButton(QPainter* painter, const QRect& rect, const QStyleOptionViewItem& option) const;
    // 检查是否点击了修复按钮
    bool isRepairButtonClicked(const QRect& rect, const QPoint& pos) const;
};

#endif // TOOLDELEGATE_H
