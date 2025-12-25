#include "richtextdelegate.h"

void RichTextDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // 绘制单元格背景
    painter->save();
    painter->fillRect(option.rect, opt.backgroundBrush);

    // 解析富文本（转换为纯文本时保留换行逻辑，或直接用QTextDocument绘制）
    QTextDocument doc;
    doc.setHtml(opt.text); // 用HTML解析富文本
    doc.setTextWidth(option.rect.width() - 4); // 减去左右边距（避免文本贴边）

    // 调整绘制位置（左对齐、顶部对齐）
    painter->translate(option.rect.left() + 2, option.rect.top() + 2); // 边距偏移
    doc.drawContents(painter);

    painter->restore();
}

QSize RichTextDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    QStyleOptionViewItem opt = option;
    initStyleOption(&opt, index);

    // 计算富文本在当前列宽下的高度
    QTextDocument doc;
    doc.setHtml(opt.text);
    doc.setTextWidth(option.rect.width() - 4); // 同绘制时的宽度（减去边距）
    return QSize(option.rect.width(), doc.size().height() + 4); // 加上上下边距
}
