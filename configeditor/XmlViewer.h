#ifndef XMLVIEWER_H
#define XMLVIEWER_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#include <QFile>
#include <QMessageBox>

class XmlViewer : public QWidget
{
    Q_OBJECT

public:
    XmlViewer(QWidget *parent = nullptr);
    ~XmlViewer() = default;

public slots:
    void loadXmlFile(const QString& filePath);
signals:
    void itemSelected(const QString& key, const QString& value);

private slots:
    void onTreeItemClicked(QTreeWidgetItem* item, int column);
private:
    void initUi();
    bool parseXml(QXmlStreamReader& reader, QTreeWidgetItem* parentItem);
    void clear();
    void setRichTextItem(QTreeWidgetItem* item, int column, const QString& text);

    QTreeWidget* m_xmlTree;
};

#endif // XMLVIEWER_H
