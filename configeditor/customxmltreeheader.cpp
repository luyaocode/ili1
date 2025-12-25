#include "customxmltreeheader.h"

#include <QEvent>
#include <QTreeWidget>

CustomXmlTreeHeader::CustomXmlTreeHeader(QTreeWidget *treeWidget, Qt::Orientation orientation)
    : QHeaderView(orientation, treeWidget)
{
}

bool CustomXmlTreeHeader::event(QEvent *e)
{
    // 通过树控件获取当前列宽，若小于200则修正
    if (dynamic_cast<QTreeWidget*>(parent())->columnWidth(0) < 200) {
        bool blocked = blockSignals(true); // 避免递归触发
        dynamic_cast<QTreeWidget*>(parent())->setColumnWidth(0, 200); // 使用setColumnWidth调整
        blockSignals(blocked);
    }
    return QHeaderView::event(e);
}
