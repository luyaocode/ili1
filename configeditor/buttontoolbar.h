#ifndef BUTTONTOOLBAR_H
#define BUTTONTOOLBAR_H

#include <QWidget>
#include <QHBoxLayout>
#include <QPushButton>
#include <functional>
#include <QStyle>

// 按钮工具栏类，支持动态添加按钮和回调
class ButtonToolBar : public QWidget {
    Q_OBJECT
public:
    // 定义回调函数类型（使用std::function支持lambda和任意函数）
    using ButtonCallback = std::function<void()>;

    explicit ButtonToolBar(QWidget* parent = nullptr);
    virtual void initControls();
    /**
     * @brief 添加按钮到工具栏
     * @param buttonName 按钮显示文本
     * @param callback 按钮点击时的回调函数
     * @param icon 按钮图标（可选）
     * @return 添加的按钮指针（可用于后续修改）
     */
    QPushButton *addButton(const QString& buttonName,
                            ButtonCallback callback,
                            const QString& toolTip = "",
                            const QIcon& icon = QIcon()
                          );

    /**
     * @brief 在指定位置插入分隔线
     * @param index 插入位置（-1表示末尾）
     */
    void addSeparator(int index = -1);

    /**
     * @brief 清空所有按钮和分隔线
     */
    void clear();
protected:
    QHBoxLayout* m_layout = nullptr; // 水平布局管理器
};

class TopButtonToolBar: public ButtonToolBar
{
    Q_OBJECT
public:
    explicit TopButtonToolBar(QWidget* parent = nullptr);
    void initControls() override;
signals:
    void sigExit();
};

#endif // BUTTONTOOLBAR_H
