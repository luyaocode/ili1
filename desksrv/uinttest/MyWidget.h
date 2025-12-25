#ifndef MYWIDGET_H
#define MYWIDGET_H
// MyWidget.h
#include <QWidget>
#include "unify/signaldebugger.hpp"
class SignalEmitter : public QObject
{
    Q_OBJECT
public:
    void clickBtn()
    {
        emit sigClickBtn();
    }
signals:
    void sigClickBtn();
};

class MyWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MyWidget(QWidget *parent = nullptr): QWidget(parent)
    {
    }

public slots:
    // 示例槽函数
    void onCustomBtnClicked()
    {
        // 核心：直接调用宏，无任何编译错误（Qt5.9.5 完美兼容）
        PRINT_SIGNAL_DETAILED_SOURCE();

        // 原有槽函数逻辑
        qDebug() << "onButtonClicked";
    }
};
#endif  // MYWIDGET_H
