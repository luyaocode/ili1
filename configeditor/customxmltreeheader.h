#ifndef CUSTOMXMLTREEHEADER_H
#define CUSTOMXMLTREEHEADER_H

#include <QHeaderView>

class QEvent;
class QTreeWidget;
// 自定义表头，限制第一列最小宽度
class CustomXmlTreeHeader : public QHeaderView
{
    Q_OBJECT
public:
    explicit CustomXmlTreeHeader(QTreeWidget *treeWidget, Qt::Orientation orientation);

protected:
    bool event(QEvent *e) override;
};
#endif // CUSTOMXMLTREEHEADER_H
