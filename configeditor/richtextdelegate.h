#ifndef RICHTEXTDELEGATE_H
#define RICHTEXTDELEGATE_H

#include <QTableWidget>
#include <QTableWidgetItem>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QTextLayout>
#include <QFontMetrics>
#include <QTextDocument>

// 自定义代理：支持富文本和自动换行
class RichTextDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    // 重写绘制方法：解析富文本并自动换行
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    // 重写尺寸提示：返回文本实际需要的高度（用于自动调整行高）
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif // RICHTEXTDELEGATE_H
