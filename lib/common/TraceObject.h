#ifndef TRACK_OBJECT_H__
#define TRACK_OBJECT_H__
#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>
#include <string>
#include <ctime>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <functional>

constexpr const int INT_DURATION_MARK = 500;

inline std::string getFileName(const std::string &fullPath)
{
    size_t lastSlashPos = fullPath.find_last_of('/');
    if (lastSlashPos != std::string::npos)
    {
        return fullPath.substr(lastSlashPos + 1);
    }
    else
    {
        return fullPath;
    }
}

class TraceObject
{
public:
    explicit TraceObject(const std::string &objName,
                         const std::string &threadName = "",
                         const std::string &file       = "",
                         const std::string &func       = "",
                         int                line       = 0)
        : m_strObjName(std::move(objName))
        , m_strThreadName(std::move(threadName))
        , m_strFile(file)
        , m_strFunc(func)
        , m_Line(line)
        , m_ThreadId(std::this_thread::get_id())
        , m_StartTime(std::chrono::high_resolution_clock::now())
    {
        printInfo("begin");
    }

    ~TraceObject()
    {
        auto endTime  = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - m_StartTime).count();
        printInfo("end");
        std::string info = "duration: " + std::to_string(duration) + " ms. " + "<" + getFileName(m_strFile) + ":" +
                           std::to_string(m_Line) + " " + m_strFunc + ">";
        std::string color;
        if (duration >= INT_DURATION_MARK)  // 如果耗时超过了阈值,则显著标记
        {
            color = "255;0;0";
        }
        printInfo(info, color);
    }

private:
    std::mutex                                     m_Mutex;
    std::string                                    m_strObjName;
    std::string                                    m_strThreadName;
    std::string                                    m_strFile;
    std::string                                    m_strFunc;
    int                                            m_Line;
    std::thread::id                                m_ThreadId;
    std::chrono::high_resolution_clock::time_point m_StartTime;

private:
    void printInfo(const std::string &opt, const std::string &color = "")
    {
        auto now     = std::chrono::system_clock::now();
        auto nowMs   = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        auto epochMs = nowMs.time_since_epoch();

        auto nowTimeT = std::chrono::system_clock::to_time_t(now);
        auto ms       = epochMs.count() % 1000;

        std::tm     localTm  = *std::localtime(&nowTimeT);
        std::string strColor = color.empty() ? getColor() : color;
        // 38;2;   38-前景色 2-24位RGB模式(3字节) strColor-rgb颜色
        std::cout << "\033[38;2;" + strColor + "m[" << m_strThreadName << " " << m_ThreadId << "] "
                  << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms
                  << " "
                  << " [libijk] " << m_strObjName << " " << opt << "\033[0m" << std::endl;
    }
    std::string getColor()
    {
        size_t hash = std::hash<std::thread::id> {}(m_ThreadId);
        int    r    = (hash >> 24) % 256;
        int    g    = (hash >> 16) % 256;
        int    b    = (hash >> 8) % 256;
        return std::to_string(r) + ";" + std::to_string(g) + ";" + std::to_string(b);
    }
};

// 粗体橙色
class OrangeLog
{
public:
    inline static void out(const std::string &info,
                           const std::string &threadName = "",
                           const std::string &file       = "",
                           const std::string &func       = "",
                           int                line       = 0)
    {
        auto now     = std::chrono::system_clock::now();
        auto nowMs   = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        auto epochMs = nowMs.time_since_epoch();

        auto nowTimeT = std::chrono::system_clock::to_time_t(now);
        auto ms       = epochMs.count() % 1000;

        std::tm     localTm = *std::localtime(&nowTimeT);
        std::string pos     = "<" + getFileName(file) + ":" + std::to_string(line) + " " + func + ">";
        // 38;5;208   38-前景色 5-256色模式 208-256色模式橙色索引
        std::cout << "\033[1;38;5;208m[" << threadName << " " << std::this_thread::get_id() << "] "
                  << std::put_time(&localTm, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << ms
                  << " "
                  << " [libijk] " << info << " " << pos << " "
                  << "\033[0m" << std::endl;
    }
};
#define CONCAT_LINE_INNER(a, b) a##b
#define CONCAT_LINE(a, b) CONCAT_LINE_INNER(a, b)
// 最终宏：使用两层中间宏确保__LINE__展开
#define TRACEOBJECT(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT2(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT3(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT4(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT5(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT6(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT7(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT8(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT9(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT10(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT11(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT12(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT13(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT14(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);

#define TRACEOBJECT15(strOpt) TraceObject CONCAT_LINE(traceObject_, __LINE__)(strOpt, "", __FILE__, __func__, __LINE__);

#define TRACEOBJECT_CU(strOpt) TraceObject traceObject_##__LINE__(strOpt, "", __FILE__, __func__, __LINE__);
#define TRACEOBJECT__CU(strOpt, strThreadName) \
    TraceObject traceObject_##__LINE__(strOpt, strThreadName, __FILE__, __func__, __LINE__);

#define ORANGELOG(strInfo) OrangeLog::out(strInfo, "", __FILE__, __func__, __LINE__);

#endif
