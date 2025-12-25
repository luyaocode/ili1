#include "XmlViewer.h"
#include <QHeaderView>
#include <QTextOption>
#include <QStyledItemDelegate>

#include "editablelabel.h"
#include "customxmltreeheader.h"

// 为QTreeWidgetItem设置富文本+自动换行的内容（支持两列）
void XmlViewer::setRichTextItem(QTreeWidgetItem* item, int column, const QString& text) {
    // 1. 创建支持富文本和自动换行的QLabel
    QLabel* label = new EditableLabel(text);
    label->setTextFormat(Qt::RichText); // 启用富文本（支持<br>、<font>等标签）
    label->setAlignment(Qt::AlignTop | Qt::AlignLeft); // 顶部对齐，避免文字居中导致的空白
    label->setMinimumHeight(20);       // 最小高度，防止内容过短时被压缩

    // 2. 用布局包裹Label，确保自动调整大小
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(2, 2, 2, 2); // 与默认item内边距一致
    layout->addWidget(label);

    // 3. 将widget设置到TreeWidgetItem的对应列
    QTreeWidget* treeWidget = item->treeWidget();
    treeWidget->setItemWidget(item, column, widget);

    // 4. 存储Label指针到item的UserData，方便后续编辑后更新
    item->setData(column, Qt::UserRole, QVariant::fromValue(label));

    // 关键：连接标签的点击信号到树节点的点击处理
    connect(dynamic_cast<EditableLabel*>(label), &EditableLabel::labelClicked,
        [=]() {
            if (item)
            {
                onTreeItemClicked(item, column);
            }
        }
    );
}

XmlViewer::XmlViewer(QWidget *parent)
    : QWidget(parent)
{
    initUi();
}

void XmlViewer::initUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_xmlTree = new QTreeWidget(this);
    m_xmlTree->setColumnCount(2);
    QHeaderView *header = new CustomXmlTreeHeader(m_xmlTree, Qt::Horizontal);
    m_xmlTree->setHeader(header);
    m_xmlTree->setHeaderLabels({"名称", "值"});
    m_xmlTree->setExpandsOnDoubleClick(true);

    m_xmlTree->setEditTriggers(QAbstractItemView::DoubleClicked); // 双击触发编辑
    m_xmlTree->setUniformRowHeights(false); // 保持关闭统一行高
    m_xmlTree->header()->setStretchLastSection(true); // 最后一列拉伸填充（可选，优化布局）
    m_xmlTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_xmlTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_xmlTree->setColumnWidth(0, 200); // 使用setColumnWidth调整

    layout->addWidget(m_xmlTree);


    // connect
    // 连接树节点点击信号到自定义槽函数
    connect(m_xmlTree, &QTreeWidget::itemClicked,
            this, &XmlViewer::onTreeItemClicked);

}

void XmlViewer::loadXmlFile(const QString& filePath)
{
    clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::warning(this, "警告", "无法打开文件：" + file.errorString());
        return;
    }

    QXmlStreamReader reader(&file);
    QTreeWidgetItem* rootItem = nullptr;

    // 解析XML根元素
    while (!reader.atEnd() && !reader.hasError())
    {
        QXmlStreamReader::TokenType token = reader.readNext();
        if (token == QXmlStreamReader::StartElement)
        {
            rootItem = new QTreeWidgetItem(m_xmlTree);
            rootItem->setText(0, reader.name().toString());
            rootItem->setIcon(0, QIcon::fromTheme("application-xml"));
            // 可编辑
            rootItem->setFlags(rootItem->flags() | Qt::ItemIsEditable);

            // 添加根元素属性
            QXmlStreamAttributes attrs = reader.attributes();
            for (const QXmlStreamAttribute& attr : attrs)
            {
                QTreeWidgetItem* attrItem = new QTreeWidgetItem(rootItem);
                // 第一列：@+属性名（支持富文本，例如可添加颜色）
                QString attrNameText = QString("<font color='#2c3e50'>@%1</font>").arg(attr.name().toString());
                setRichTextItem(attrItem, 0, attrNameText);

                // 第二列：属性值（自动换行，支持富文本格式）
                QString attrValueText = attr.value().toString();
                // 可选：给长文本添加换行提示，或直接使用原始文本（QLabel会自动根据列宽换行）
                setRichTextItem(attrItem, 1, attrValueText);
                // 可编辑
                attrItem->setFlags(attrItem->flags() | Qt::ItemIsEditable);
            }

            // 递归解析子元素
            if (!parseXml(reader, rootItem))
            {
                QMessageBox::warning(this, "警告", "XML解析错误：" + reader.errorString());
                break;
            }
        }
    }

    file.close();
    m_xmlTree->expandAll();
}

bool XmlViewer::parseXml(QXmlStreamReader& reader, QTreeWidgetItem* parentItem)
{
    while (!reader.atEnd())
    {
        QXmlStreamReader::TokenType token = reader.readNext();

        switch (token)
        {
            case QXmlStreamReader::StartElement:
            {
                QTreeWidgetItem* childItem = new QTreeWidgetItem(parentItem);
                childItem->setText(0, reader.name().toString());
                // 可编辑
                childItem->setFlags(childItem->flags() | Qt::ItemIsEditable);

                // 添加元素属性
                QXmlStreamAttributes attrs = reader.attributes();
                for (const QXmlStreamAttribute& attr : attrs)
                {
                    QTreeWidgetItem* attrItem = new QTreeWidgetItem(childItem);
                    // 可编辑
                    attrItem->setFlags(attrItem->flags() | Qt::ItemIsEditable);

                    // 第一列：@+属性名（支持富文本，例如可添加颜色）
                    QString attrNameText = QString("<font color='#2c3e50'>@%1</font>").arg(attr.name().toString());
                    setRichTextItem(attrItem, 0, attrNameText);

                    // 第二列：属性值（自动换行，支持富文本格式）
                    QString attrValueText = attr.value().toString();
                    // 可选：给长文本添加换行提示，或直接使用原始文本（QLabel会自动根据列宽换行）
                    setRichTextItem(attrItem, 1, attrValueText);
                }

                // 递归解析子元素
                if (!parseXml(reader, childItem))
                    return false;
                break;
            }

            case QXmlStreamReader::EndElement:
                return true;

            case QXmlStreamReader::Characters:
            {
                QString text = reader.text().toString().trimmed();
                if (!text.isEmpty())
                {
                    parentItem->setText(1, text);
                }
                break;
            }

            case QXmlStreamReader::Comment:
            {
                QTreeWidgetItem* commentItem = new QTreeWidgetItem(parentItem);
                QString strCommentAttr = QString("<font color='#2c3e50'>@comment</font>");
                QString strCommentVal = QString("<font color='#2c3e50'>%1</font>").arg(reader.text().toString());
                setRichTextItem(commentItem,0,strCommentAttr);
                setRichTextItem(commentItem,1,strCommentVal);
                break;
            }

            default:
                break;
        }
    }

    return !reader.hasError();
}

void XmlViewer::clear()
{
    m_xmlTree->clear();
}

// 树节点点击处理
void XmlViewer::onTreeItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);  // 忽略点击的列索引，始终取第一列和第二列
    if (item) {
        QLabel* lblAttr = item->data(0, Qt::UserRole).value<QLabel*>();
        QLabel* lblVal = item->data(1, Qt::UserRole).value<QLabel*>();
        if(lblAttr && lblVal)
        {
            emit itemSelected(lblAttr->text(), lblVal->text());
        }
    }
}
