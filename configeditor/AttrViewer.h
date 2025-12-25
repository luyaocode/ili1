#ifndef ATTRVIEWER_H
#define ATTRVIEWER_H

#include <QTableWidget>

// 属性查看器控件，用于展示键值对形式的属性信息
class AttrViewer : public QTableWidget
{
    Q_OBJECT
public:
    explicit AttrViewer(QWidget *parent = nullptr);

    // 添加一条属性信息（键值对）
    void addProperty(const QString &key, const QString &value);

    // 清空所有属性信息（保留表头）
    void clearProperties();

    // 设置表头文本（默认"属性"和"值"）
    void setHeaderLabels(const QString &keyLabel, const QString &valueLabel);
public slots:
    void slotData(const QString &key, const QString &value);
private:
    // 设置富文本单元格（修复viewOptions()调用问题）
    void setRichTextCell(int row, int column, const QString& richText);
};

#endif // ATTRVIEWER_H
