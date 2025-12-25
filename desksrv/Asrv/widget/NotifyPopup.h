#ifndef NOTIFYPOPUP_H
#define NOTIFYPOPUP_H

#include <QWidget>

class NotifyPopup : public QWidget
{
    Q_OBJECT

public:
    explicit NotifyPopup(const QString &title, const QString &message, QWidget *parent = nullptr);

protected:
    // 声明事件过滤器函数
    bool eventFilter(QObject *watched, QEvent *event) override;

};

#endif // NOTIFYPOPUP_H
