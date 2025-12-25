#ifndef COMMONDELEGATE_H
#define COMMONDELEGATE_H

#include <QStyledItemDelegate>
#include <QMouseEvent>

class CommonDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    enum ButtonType
    {
        TerminalBtn = 0,  // 打开终端
        DirectoryBtn      // 打开目录
    };

    explicit CommonDelegate(QObject *parent = nullptr);

    // 重写绘制函数
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    // 重写大小提示（保证按钮有足够空间）
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    // 捕获鼠标点击事件
    bool editorEvent(QEvent                     *event,
                     QAbstractItemModel         *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex          &index) override;
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override;

signals:
    // 按钮点击信号：传递行号和按钮类型
    void btnClicked(int row, ButtonType type);

private:
    // 计算按钮区域（每个按钮占一半宽度）
    QRect getBtnRect(const QStyleOptionViewItem &option, ButtonType type) const;
    void  drawCustomButton(QPainter                   *painter,
                           const QStyleOptionViewItem &option,
                           const QRect                &btnRect,
                           const QString              &btnText) const;
};

#endif  // COMMONDELEGATE_H
