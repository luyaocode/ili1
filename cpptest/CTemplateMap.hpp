#include <type_traits>
#include <utility>
#include <cstddef>

// 前向声明
template <typename... Pairs>
struct Map;

// 空Map的大小特化
template <>
struct Map<> {
    static constexpr size_t size = 0;
};
// 键值对结构
template <typename Key, typename Value>
struct Pair {
    using key = Key;
    using value = Value;
};

// 辅助工具：检查类型是否为Pair
template <typename T>
struct is_pair : std::false_type {};

template <typename K, typename V>
struct is_pair<Pair<K, V>> : std::true_type {};

// 辅助工具：查找键值对
template <typename Key, typename... Pairs>
struct find_pair;

// 找到匹配的键
template <typename Key, typename Value, typename... Rest>
struct find_pair<Key, Pair<Key, Value>, Rest...> {
    using type = Pair<Key, Value>;
};

// 继续查找
template <typename Key, typename OtherKey, typename OtherValue, typename... Rest>
struct find_pair<Key, Pair<OtherKey, OtherValue>, Rest...> {
    using type = typename find_pair<Key, Rest...>::type;
};

// 未找到键
template <typename Key>
struct find_pair<Key> {
    // 可以定义一个特殊类型表示未找到
    struct not_found {};
    using type = not_found;
};

// 检查键是否存在
template <typename Key, typename... Pairs>
struct has_key {
    static constexpr bool value = !std::is_same<
        typename find_pair<Key, Pairs...>::type::not_found,
        typename find_pair<Key, Pairs...>::type
    >::value;
};

// 主Map实现
template <typename First, typename... Rest>
struct Map<First, Rest...> {
    static_assert(is_pair<First>::value, "All template arguments must be Pair types");
    
    // 获取第一个键值对
    using first = First;
    using key = typename first::key;
    using value = typename first::value;
    
    // 剩余的Map
    using rest = Map<Rest...>;
    
    // 查找键对应的值
    template <typename Key>
    struct get {
        using type = typename find_pair<Key, First, Rest...>::type::value;
    };
    
    // 检查键是否存在
    template <typename Key>
    struct contains {
        static constexpr bool value = has_key<Key, First, Rest...>::value;
    };
    
    // 大小（键值对数量）
    static constexpr size_t size = 1 + rest::size;
};


// 辅助函数：获取值类型
template <typename Key, typename... Pairs>
using get_value_t = typename Map<Pairs...>::template get<Key>::type;

// 辅助函数：检查键是否存在
template <typename Key, typename... Pairs>
constexpr bool has_key_v = Map<Pairs...>::template contains<Key>::value;

// 辅助宏：简化键值对定义
#define MAP_PAIR(Key, Value) Pair<Key, Value>

// 使用示例：

// 定义一些类型作为键
struct NameKey {};
struct AgeKey {};
struct WeightKey {};

// 创建编译期Map
using MyMap = Map<
    MAP_PAIR(NameKey, const char*),
    MAP_PAIR(AgeKey, int),
    MAP_PAIR(WeightKey, double)
>;

// 编译期测试
static_assert(MyMap::size == 3, "Map size should be 3");
static_assert(has_key_v<NameKey, 
    MAP_PAIR(NameKey, const char*),
    MAP_PAIR(AgeKey, int)>, "Map should contain NameKey");
static_assert(std::is_same<get_value_t<AgeKey,
    MAP_PAIR(NameKey, const char*),
    MAP_PAIR(AgeKey, int)>, int>::value, "AgeKey should map to int");

