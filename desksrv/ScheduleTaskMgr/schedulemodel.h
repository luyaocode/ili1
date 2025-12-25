#ifndef SCHDUELMODEL_H
#define SCHDUELMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include "scheduleitem.h"

class ScheduleModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    enum Columns
    {
        NameColumn = 0,     // 名称,xxx
        TaskStatusColumn,   // 任务状态 , 启用/禁用
        TaskTypeColumn,     // 任务类型, 时间点/间隔
        TriggerTimeColumn,  // 触发时机, 时间点列表/间隔
        PathStatusColumn,   // 任务路径状态 ,存在/不存在
        PathColumn,         // 任务路径, /usr/bin/xxx
        ColumnCount         // 总列数
    };

    explicit ScheduleModel(QObject *parent = nullptr);
    ~ScheduleModel();

    // 重写模型接口
    int           rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int           columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant      data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant      headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool          setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    // 加载/保存配置
    void                       loadConfig(QList<ScheduleItem> list);
    const QList<ScheduleItem> &getData() const
    {
        return m_data;
    }
signals:
    void sigSaveConfig(const QList<ScheduleItem>& items);

private:
    QList<ScheduleItem> m_data;
};

#endif  // SCHDUELMODEL_H
