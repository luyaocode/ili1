#ifndef EDITABLELABEL_H
#define EDITABLELABEL_H

#include <QLabel>
#include <QLineEdit>
#include <QEvent>
#include <QScreen>
#include <QApplication>

class EditableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit EditableLabel(QWidget *parent = nullptr);
    explicit EditableLabel(const QString &text, QWidget *parent = nullptr);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    // 重写鼠标点击事件
    void mousePressEvent(QMouseEvent *event) override;
signals:
    // 新增：当标签被点击时发送信号
    void labelClicked();
private slots:
    void onEditingFinished();

private:
    void init();
    QLineEdit *m_lineEdit;
};

#endif // EDITABLELABEL_H
