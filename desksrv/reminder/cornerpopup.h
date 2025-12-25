#ifndef CORNERPOPUN_H
#define CORNERPOPUN_H

#include <QWidget>
class CornerPopup : public QWidget
{
    Q_OBJECT
public:
    // 构造函数：接收要显示的文本
    explicit CornerPopup(const QString &text, QWidget *parent = nullptr);

private:
    // 移动窗口到右下角
    void moveToBottomRight();
};

#endif
