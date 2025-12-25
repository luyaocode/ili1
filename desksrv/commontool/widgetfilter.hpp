#ifndef WIDGETFILTER_HPP
#define WIDGETFILTER_HPP
#include <QWidget>
#include <QObject>
#include <QList>
#include <QGroupBox>
#include <QString>
#include <QRegularExpression>
#include <type_traits>

// -------------------------- 全局编译期类型校验模板（移到函数外部） --------------------------
// 空变参包校验（递归终止）
template<typename... Args>
struct AllDerivedFromQWidget
{
    static const bool value = true;
};

// 单类型校验
template<typename T>
struct AllDerivedFromQWidget<T>
{
    static const bool value = std::is_base_of<QWidget, T>::value;
};

// 多类型递归校验（C++11 兼容）
template<typename First, typename... Rest>
struct AllDerivedFromQWidget<First, Rest...>
{
    static const bool value = std::is_base_of<QWidget, First>::value && AllDerivedFromQWidget<Rest...>::value;
};

// 过滤结构体（包含类型ID列表）
struct WidgetFilter
{
    enum MatchMode
    {
        Include,
        Exclude
    } mode = Exclude;
    QString    namePattern;       // 名称过滤（空=不过滤）
    bool       useRegex = false;  // 名称是否用正则
    QList<int> typeIds;           // 要过滤的控件元类型ID列表
};

// -------------------------- 修复后的递归辅助函数（添加类型ID到过滤规则） --------------------------
// 1. 终止函数：无变参时调用（最终递归终止）
inline void addTypeIdsToFilter(WidgetFilter & /*filter*/)
{
}

// 2. 单类型处理函数（处理1个类型，避免空变参包）
template<typename First>
void addTypeIdsToFilter(WidgetFilter &filter)
{
    // 编译期校验：必须是 QWidget 派生类
    static_assert(std::is_base_of<QWidget, First>::value, "createTypeFilter：控件类型必须是 QWidget 派生类！");
    // 添加元类型ID（Qt标准控件已默认注册，自定义控件需手动注册）
    filter.typeIds.append(qMetaTypeId<First *>());
}

// 3. 多类型递归函数（处理2个及以上类型）
template<typename First, typename Second, typename... Rest>
void addTypeIdsToFilter(WidgetFilter &filter)
{
    // 编译期校验当前类型
    static_assert(std::is_base_of<QWidget, First>::value, "createTypeFilter：控件类型必须是 QWidget 派生类！");
    // 添加当前类型的元类型ID
    filter.typeIds.append(qMetaTypeId<First *>());
    // 递归处理剩余类型（始终保留Second，避免空变参包）
    addTypeIdsToFilter<Second, Rest...>(filter);
}

/**
 * @brief 快速创建「按类型过滤」的规则（兼容 C++11/C++14）
 * @tparam Args 控件类型列表（必须是 QWidget 派生类，如 QPushButton, QLineEdit）
 * @param mode 匹配模式（Include/Exclude）
 * @return WidgetFilter 过滤规则
 */
template<typename... Args>
WidgetFilter createTypeFilter(WidgetFilter::MatchMode mode)
{
    // 编译期断言：至少指定一个类型
    static_assert(sizeof...(Args) > 0, "createTypeFilter：至少需要指定一个控件类型！");
    // 编译期断言：所有类型都是 QWidget 派生类（使用全局模板，避免局部类问题）
    static_assert(AllDerivedFromQWidget<Args...>::value, "createTypeFilter：所有类型必须是 QWidget 派生类！");

    WidgetFilter filter;
    filter.mode = mode;
    addTypeIdsToFilter<Args...>(filter);  // 调用递归辅助函数
    return filter;
}

/**
 * @brief 递归启用/禁用界面所有控件（支持类型+名称过滤）
 * @param parentWidget 父控件（QWidget派生类）
 * @param enable true=启用，false=禁用
 * @param filter 过滤规则（默认无过滤）
 */
template<typename T = QWidget>
void setWidgetEnabledRecursively(T *parentWidget, bool enable, const WidgetFilter &filter = WidgetFilter())
{
    // 校验父控件有效性
    if (!parentWidget || !parentWidget->isWidgetType())
    {
        return;
    }

    // -------------------------- 步骤1：判断是否需要过滤 --------------------------
    bool shouldProcess = true;

    // 1.1 类型过滤（基于元类型ID）
    if (!filter.typeIds.isEmpty())
    {
        // 关键修复：通过 Qt 的 metaObject() 获取控件实际类型
        const QMetaObject *metaObj = parentWidget->metaObject();
        // 获取实际类型的「指针元类型ID」（与 createTypeFilter 中 qMetaTypeId<First*> 完全一致）
        int         widgetTypeId = qMetaTypeId<void *>();  // 临时初始化
        const char *className    = metaObj->className();   // 实际类型名（如 "QPushButton"）

        // 核心：通过类型名动态获取元类型ID（Qt元对象系统自动匹配）
        // 原理：qMetaTypeIdFromString 能识别 "QPushButton*" 这类指针类型名
        QString typeName = QString("%1*").arg(className);  // 构造指针类型名（如 "QPushButton*"）
        widgetTypeId     = QMetaType::type(typeName.toUtf8().constData());

        bool typeMatched = filter.typeIds.contains(widgetTypeId);

        // 根据匹配模式更新是否处理
        if ((filter.mode == WidgetFilter::Include && !typeMatched) ||
            (filter.mode == WidgetFilter::Exclude && typeMatched))
        {
            shouldProcess = false;
        }
    }

    // 1.2 名称过滤
    if (!filter.namePattern.isEmpty())
    {
        const QString widgetName  = parentWidget->objectName();
        bool          nameMatched = false;

        if (filter.useRegex)
        {
            QRegularExpression regex(filter.namePattern, QRegularExpression::CaseInsensitiveOption);
            nameMatched = regex.match(widgetName).hasMatch();
        }
        else
        {
            nameMatched = widgetName.contains(filter.namePattern, Qt::CaseInsensitive);
        }

        // 根据匹配模式更新是否处理
        if ((filter.mode == WidgetFilter::Include && !nameMatched) ||
            (filter.mode == WidgetFilter::Exclude && nameMatched))
        {
            shouldProcess = false;
        }
    }

    // -------------------------- 步骤2：处理当前控件 --------------------------
    if (shouldProcess)
    {
        parentWidget->setEnabled(enable);

        // 可选视觉优化：QGroupBox禁用时扁平化
        if (auto groupBox = qobject_cast<QGroupBox *>(parentWidget))
        {
            groupBox->setFlat(!enable);
        }
    }

    // -------------------------- 步骤3：递归处理所有子控件 --------------------------
    for (QObject *child : parentWidget->children())
    {
        if (auto childWidget = qobject_cast<QWidget *>(child))
        {
            setWidgetEnabledRecursively(childWidget, enable, filter);
        }
    }
}
#endif  // WIDGETFILTER_HPP
