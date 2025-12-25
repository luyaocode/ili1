#include "commonmodel.h"
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

CommonModel::CommonModel(QObject *parent): QAbstractTableModel(parent)
{
}

CommonModel::~CommonModel()
{
}

int CommonModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_listCommon.size();
}

int CommonModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

QVariant CommonModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_listCommon.size() || index.column() >= ColumnCount)
        return QVariant();

    const QuickItem &item   = m_listCommon[index.row()];
    auto             enable = item.enable;
    switch (role)
    {
        case Qt::DisplayRole:
        case Qt::EditRole:
            switch (index.column())
            {
                case NameColumn:
                    return item.name;
                case PathColumn:
                    return item.path;
                case ShortcutColumn:
                    return item.shortcut;
                case StatusColumn:
                    return item.isValid ? tr("是") : QString("否");
                default:
                    return QVariant();
            }
        case Qt::TextAlignmentRole:
            return Qt::AlignCenter;
        case Qt::UserRole:  // 用于传递是否有效状态给委托
            if (index.column() == StatusColumn)
                return item.isValid;
            break;
            // 新增：文字颜色（禁用时灰色）
        case Qt::ForegroundRole:
            if (!enable)
                return QColor(Qt::gray);  // 文字变灰色（也可用 #888888 等深灰色，根据需求调整）
            break;
        // 新增：背景颜色（可选，禁用时浅灰色背景，增强视觉效果）
        case Qt::BackgroundRole:
            if (!enable)
                return QColor(0xF5, 0xF5, 0xF5);  // 浅灰色背景（接近默认背景，不刺眼）
            break;
        default:
            return QVariant();
    }
    return QVariant();
}

QVariant CommonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch (section)
        {
            case NameColumn:
                return tr("名称");
            case PathColumn:
                return tr("路径");
            case StatusColumn:
                return tr("是否存在");
            case OperationColunm:
                return tr("操作");
            case ShortcutColumn:
                return tr("快捷键");
            default:
                return QVariant();
        }
    }
    return QVariant();
}

Qt::ItemFlags CommonModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    Qt::ItemFlags flags = QAbstractTableModel::flags(index);
    int           row   = index.row();
    if (row < 0 || row >= m_listCommon.size())
        return flags;  // 行越界时，仅返回基础标志（不可编辑）
    // 限制编辑
    if (m_listCommon[row].enable && (index.column() == ShortcutColumn || index.column() == PathColumn))
        flags |= Qt::ItemIsEditable;
    if (index.column() == OperationColunm)
        flags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    return flags;
}

bool CommonModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_listCommon.size() || role != Qt::EditRole)
        return false;

    bool changed = false;
    if (index.column() == ShortcutColumn)
    {
        // 更新快捷键
        QString shortcutStr = value.toString().trimmed();
        if (m_listCommon[index.row()].shortcut != shortcutStr)
        {
            m_listCommon[index.row()].shortcut = shortcutStr;
            changed                            = true;
        }
    }
    else if (index.column() == PathColumn)
    {
        // 更新路径
        QString newPath = value.toString().trimmed();
        if (m_listCommon[index.row()].path != newPath)
        {
            m_listCommon[index.row()].path = newPath;
            beginResetModel();
            m_listCommon[index.row()].isValid = Commontool::checkPathExist(newPath);
            endResetModel();
            changed = true;
        }
    }
    if (!changed)
    {
        return false;
    }
    emit sigSaveConfig();
    emit sigUpdateKeyBlockers(m_listCommon.mid(index.row(), 1));

    emit dataChanged(index, index);
    return true;
}

void CommonModel::loadConfig(QList<QuickItem> list)
{
    beginResetModel();
    m_listCommon = list;
    endResetModel();
}
