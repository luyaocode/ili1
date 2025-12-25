#ifndef DYNAMIC_LIBRARY_HPP
#define DYNAMIC_LIBRARY_HPP

#include <iostream>
#include <string>
#include <stdexcept>
#include <type_traits>
#include <dlfcn.h>

// 禁用拷贝，仅允许移动（避免句柄重复释放）
#define DISABLE_COPY(CLASS)                   \
    CLASS(const CLASS &)            = delete; \
    CLASS &operator=(const CLASS &) = delete;

/**
 * @brief Linux下动态库加载模板类
 * 特性：类型安全、自动卸载、移动语义、异常错误处理
 */
class DynamicLibrary
{
public:
    // 句柄类型（dlopen返回void*）
    using Handle = void *;

    // 构造函数：空句柄
    DynamicLibrary(): m_handle(nullptr), m_path("")
    {
    }

    // 构造函数：直接加载动态库
    explicit DynamicLibrary(const std::string &path): m_handle(nullptr), m_path("")
    {
        load(path);
    }

    // 移动构造
    DynamicLibrary(DynamicLibrary &&other) noexcept
    {
        m_handle       = other.m_handle;
        m_path         = std::move(other.m_path);
        other.m_handle = nullptr;
        other.m_path.clear();
    }

    // 移动赋值
    DynamicLibrary &operator=(DynamicLibrary &&other) noexcept
    {
        if (this != &other)
        {
            unload();  // 释放当前句柄
            m_handle       = other.m_handle;
            m_path         = std::move(other.m_path);
            other.m_handle = nullptr;
            other.m_path.clear();
        }
        return *this;
    }

    // 析构：自动卸载动态库
    ~DynamicLibrary()
    {
        unload();
    }

    /**
     * @brief 加载动态库
     * @param path 库路径（如./libcalc.so）
     */
    bool load(const std::string &path)
    {
        // 卸载已加载的库
        if (is_loaded())
        {
            unload();
        }

        // 清空dlerror（避免旧错误干扰）
        dlerror();

        // 加载库：RTLD_LAZY=延迟解析符号，RTLD_LOCAL=避免符号污染
        m_handle        = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
        const char *err = dlerror();
        if (!m_handle || err != nullptr)
        {
            fprintf(stderr, "加载库失败[%s] %s", path.c_str(), err);
            return false;
        }
        m_path = path;
        return true;
    }

    /**
     * @brief 卸载动态库
     */
    void unload()
    {
        if (is_loaded())
        {
            dlclose(m_handle);
            m_handle = nullptr;
            m_path.clear();
        }
    }

    /**
     * @brief 获取动态库中的函数（类型安全）
     * @tparam FuncType 函数指针类型（需与导出函数签名一致）
     * @param func_name 函数名（需与动态库导出名一致）
     * @return 函数指针
     */
    template<typename FuncType>
    FuncType get_function(const std::string &func_name)
    {
        static_assert(std::is_function<typename std::remove_pointer<FuncType>::type>::value,
                      "FuncType must be a function pointer type");

        if (!is_loaded())
        {
            return nullptr;
        }

        // 清空dlerror
        dlerror();

        // 获取函数地址
        void       *func_ptr = dlsym(m_handle, func_name.c_str());
        const char *err      = dlerror();
        if (func_ptr == nullptr || err != nullptr)
        {
            return nullptr;
        }

        return reinterpret_cast<FuncType>(func_ptr);
    }

    /**
     * @brief 检查库是否已加载
     */
    bool is_loaded() const noexcept
    {
        return m_handle != nullptr;
    }

    /**
     * @brief 获取当前加载的库路径
     */
    std::string get_path() const noexcept
    {
        return m_path;
    }

private:
    Handle      m_handle;         // 动态库句柄
    std::string m_path;           // 库路径
    DISABLE_COPY(DynamicLibrary)  // 禁用拷贝
};

#undef DISABLE_COPY

#endif  // DYNAMIC_LIBRARY_HPP
