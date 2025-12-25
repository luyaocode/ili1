#include "schedulemodel.h"
#include "scheduleitem.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFileInfo>
#include <QDebug>

#include "commontool/commontool.h"
#include "commontool/keyblocker.h"
#include "commontool/x11struct.h"
#include "tool.h"

ScheduleModel::ScheduleModel(QObject *parent): QAbstractTableModel(parent)
{
}

ScheduleModel::~ScheduleModel()
{
}

int ScheduleModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_data.size();
}

int ScheduleModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant ScheduleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_data.size() || index.column() >= ColumnCount)
        return QVariant();

    const ScheduleItem &item       = m_data[index.row()];
    auto                enable     = item.enable;
    bool                isEditable = (this->flags(index) & Qt::ItemIsEditable);
    switch (role)
    {
        case Qt::DisplayRole:
        case Qt::EditRole:
            switch (index.column())
            {
                case NameColumn:
                    return item.taskConfig.taskName;
                case TaskTypeColumn:
                    return taskTypeToString(item.taskConfig.type);
                case TaskStatusColumn:
                    return taskStatusToString(item.enable);
                case TriggerTimeColumn:
                {
                    if (item.taskConfig.type == TaskType::Interval)
                    {
                        return triggerTimeToString(item.taskConfig.intervalSec);
                    }
                    else
                    {
                        return triggerTimeToString(item.taskConfig.timePoints);
                    }
                }
                case PathColumn:
                    return item.path;
                case PathStatusColumn:
                    return Commontool::checkPathExist(item.path);

                default:
                    return QVariant();
            }
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::UserRole:  // 用于传递是否有效状态给委托
            if (index.column() == PathStatusColumn)
                return item.isValid;
            break;
        case Qt::ForegroundRole:
            if (!enable || !isEditable)
                return QColor(Qt::darkGray);
            return QColor(Qt::blue);
            break;
        // 新增：背景颜色（可选，禁用时浅灰色背景，增强视觉效果）
        case Qt::BackgroundRole:
            if (!enable)
                return QColor(0xB5, 0xB5, 0xB5);
            break;
        default:
            return QVariant();
    }
    return QVariant();
}

QVariant ScheduleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
            case NameColumn:
                return tr("名称");
            case TaskStatusColumn:
                return tr("任务状态");
            case TaskTypeColumn:
                return tr("任务类型");
            case TriggerTimeColumn:
                return tr("触发时机");
            case PathStatusColumn:
                return tr("任务路径状态");
            case PathColumn:
                return tr("任务路径");
            default:
                return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags ScheduleModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    int           row   = index.row();
    if (row < 0 || row >= m_data.size())
        return flags;  // 行越界时，仅返回基础标志（不可编辑）
    // 限制编辑
    QList<int> edits {Columns::TriggerTimeColumn, Columns::PathColumn};
    if (m_data[row].enable && (edits.contains(index.column())))
        flags |= Qt::ItemIsEditable;
    return flags;
}

bool ScheduleModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_data.size() || role != Qt::EditRole)
        return false;

    bool changed = false;
    if (index.column() == PathColumn)
    {
        // 更新路径
        QString newPath = value.toString().trimmed();
        if (m_data[index.row()].path != newPath)
        {
            beginResetModel();
            m_data[index.row()].path                = newPath;
            m_data[index.row()].isValid             = Commontool::checkPathExist(newPath);
            m_data[index.row()].taskConfig.callback = [newPath]() {
                if (newPath.isEmpty())
                {
                    return;
                }
                QStringList list = newPath.split(' ');
                QString     cmd  = list[0];
                QStringList params;
                if (list.size() > 1)
                {
                    params = list.mid(1);
                }
                Commontool::executeThirdProcess(cmd, params);
            };
            endResetModel();
            changed = true;
        }
    }
    else if (index.column() == Columns::TriggerTimeColumn)
    {
        // 更新触发时机
        QString newTime = value.toString().trimmed();
        if (m_data[index.row()].taskConfig.type == TaskType::Interval)
        {
            bool ok;
            int  newInterval = newTime.toUInt(&ok);
            if (!ok)
            {
                return false;
            }
            if (newInterval != m_data[index.row()].taskConfig.intervalSec)
            {
                beginResetModel();
                m_data[index.row()].taskConfig.intervalSec = newInterval;
                endResetModel();
                changed = true;
            }
        }
        else if (m_data[index.row()].taskConfig.type == TaskType::TimePoint)
        {
            QList<QTime> list;
            QStringList  splits = value.toString().trimmed().split(';');
            for (const QString &timeStr : splits)
            {
                QString trimmedStr = timeStr.trimmed();
                if (trimmedStr.isEmpty())
                {
                    continue;
                }

                QTime time = QTime::fromString(trimmedStr, "HH:mm:ss");
                if (time.isValid())
                {
                    list.append(time);
                }
                else
                {
                    qWarning() << "无效的时间格式：" << trimmedStr << "，已跳过";
                    continue;
                }
            }

            if (!Commontool::isTimeListsContentEqual(list, m_data[index.row()].taskConfig.timePoints))
            {
                beginResetModel();
                m_data[index.row()].taskConfig.timePoints = list;
                endResetModel();
                changed = true;
            }
        }
    }
    if (!changed)
    {
        return false;
    }
    emit sigSaveConfig({m_data[index.row()]});
    return true;
}

void ScheduleModel::loadConfig(QList<ScheduleItem> list)
{
    beginResetModel();
    m_data = list;
    endResetModel();
}
