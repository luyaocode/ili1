#ifndef COMMONMODEL_H
#define COMMONMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include "quickitem.h"

struct x11struct;
struct KeyWithModifier;
class KeyBlocker;
class CommonModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Columns
    {
        NameColumn = 0,
        PathColumn,
        StatusColumn,
        OperationColunm,
        ShortcutColumn,
        ColumnCount
    };

    explicit CommonModel(QObject *parent = nullptr);
    ~CommonModel();

    // 重写模型接口
    int           rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int           columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant      data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant      headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool          setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // 加载/保存配置
    void loadConfig(QList<QuickItem> list);
    const QList<QuickItem> &getData() const
    {
        return m_listCommon;
    }
signals:
    void sigUpdateKeyBlockers(const QList<QuickItem> &list);
    void sigSaveConfig();
private:
    QList<QuickItem> m_listCommon;
};

#endif // COMMONMODEL_H
