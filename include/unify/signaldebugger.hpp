#pragma once

#include <QObject>
#include <QMetaMethod>
#include <QDebug>
#include <QString>
#include <QByteArray>
#include <QThread>
#include <QFileInfo>
#include <QProcess>
#include <QCoreApplication>

// Linux 调用栈解析头文件
#include <execinfo.h>
#include <unistd.h>
#include <cxxabi.h>
#include <stdlib.h>

/**
 * @brief 解析 Linux 调用栈（直接解析原始栈帧）
 * @param stackDepth 栈深度（默认15）
 * @param skipFrames 跳过前N帧（默认2，跳过工具内部调用）
 * @return 格式化的栈信息字符串
 */
inline QString parseLinuxCallStack(int stackDepth = 15, int skipFrames = 2)
{
    stackDepth += skipFrames;
    QStringList stackLines;
    void       *callstack[stackDepth];
    int         frameCount = backtrace(callstack, stackDepth);
    char      **symbols    = backtrace_symbols(callstack, frameCount);

    if (!symbols)
        return "调用栈获取失败！";

    // ========== 获取可执行文件绝对路径 ==========
    char    exePath[1024] = {0};
    ssize_t exePathLen    = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    QString exeAbsolutePath =
        (exePathLen > 0) ? QFileInfo(exePath).absoluteFilePath() : QCoreApplication::applicationFilePath();

    // ========== 预检查调试信息 ==========

    for (int i = skipFrames; i < frameCount; ++i)
    {
        QString    rawFrame   = symbols[i];
        QByteArray frameBytes = rawFrame.toLocal8Bit();
        char      *framePtr   = frameBytes.data();

        // 1. 提取精确内存地址（兼容两种格式）
        QString addrStr = "0x0";
        // 格式1：StegoTool() [0x42087d] → 提取 0x42087d
        char *addrBracketStart = strchr(framePtr, '[');
        char *addrBracketEnd   = strchr(framePtr, ']');
        if (addrBracketStart && addrBracketEnd)
        {
            *addrBracketEnd = '\0';
            addrStr         = QString(addrBracketStart + 1).trimmed();
        }
        // 格式2：libQt5Core.so.5(_ZN7QObject5eventEP6QEvent+0xc1) [0x7ffb21707321]
        else
        {
            char *plusPos   = strchr(framePtr, '+');
            char *leftParen = strchr(framePtr, '(');
            if (plusPos && leftParen)
            {
                // 计算实际地址：基地址 + 偏移
                char *libAddrStart = strrchr(framePtr, '[');
                char *libAddrEnd   = strrchr(framePtr, ']');
                if (libAddrStart && libAddrEnd)
                {
                    *libAddrEnd = '\0';
                    addrStr     = QString(libAddrStart + 1).trimmed();
                }
            }
        }

        // 2. 解析函数名（增强还原逻辑）
        QString funcName   = "未知函数";
        char   *leftParen  = strchr(framePtr, '(');
        char   *rightParen = strchr(framePtr, ')');
        if (leftParen && rightParen && rightParen > leftParen + 1)
        {
            *rightParen        = '\0';
            QByteArray mangled = QByteArray(leftParen + 1).trimmed();
            mangled            = mangled.left(mangled.indexOf('+')).trimmed();  // 移除偏移
            if (!mangled.isEmpty())
            {
                int   status    = -1;
                char *demangled = abi::__cxa_demangle(mangled.data(), nullptr, nullptr, &status);
                if (status == 0 && demangled)
                {
                    funcName = demangled;
                    free(demangled);
                }
                else
                {
                    funcName = mangled;
                }
            }
        }

        // 3. 关键：调用 addr2line 解析源码位置（仅业务代码）
        QString srcFile        = "未知文件";
        QString srcLine        = "0";
        bool    isBusinessCode = rawFrame.contains("StegoTool") && !rawFrame.contains("libQt5");
        if (isBusinessCode && !addrStr.isEmpty() && addrStr != "0x0" && QFile::exists(exeAbsolutePath))
        {
            // 构造 addr2line 命令（同步执行，避免异步问题）
            QString cmd  = QString("addr2line -e \"%1\" -f -C -i %2 2>/dev/null").arg(exeAbsolutePath).arg(addrStr);
            FILE   *pipe = popen(cmd.toLocal8Bit().data(), "r");
            if (pipe)
            {
                char buf[1024] = {0};
                // 第一行：函数名（验证）
                if (fgets(buf, sizeof(buf), pipe))
                {
                    QString demangledFunc = QString(buf).trimmed();
                    if (!demangledFunc.isEmpty() && demangledFunc != "??")
                    {
                        funcName = demangledFunc;  // 覆盖之前的函数名
                    }
                }
                // 第二行：文件:行号
                if (fgets(buf, sizeof(buf), pipe))
                {
                    QString fileLine = QString(buf).trimmed();
                    if (!fileLine.contains("??:0") && !fileLine.isEmpty())
                    {
                        QStringList parts = fileLine.split(':');
                        if (parts.size() >= 2)
                        {
                            srcFile = QFileInfo(parts[0]).fileName();  // 简化为文件名
                            // srcFile = parts[0]; // 如需完整路径，启用此行
                            srcLine = parts[1].trimmed();
                        }
                    }
                }
                pclose(pipe);
            }
        }

        // 4. 区分 Qt 内部/业务代码
        bool isQtInternal = funcName.contains("QMetaObject::activate") || funcName.contains("QObject::qt_metacall") ||
                            funcName.contains("QCoreApplication::exec") || funcName.contains("QWidget::event") ||
                            rawFrame.contains("libQt5");

        // 5. 格式化输出（核心：添加源码位置）
        QString line = QString("  #%1: %2 %3 | 模块：%4 | 地址：%5 | 源码：%6:%7")
                           .arg(i - skipFrames, 2)
                           .arg(isQtInternal ? "[Qt内部]" : "[业务代码]")
                           .arg(funcName)
                           .arg(QFileInfo(rawFrame.section('/', -1, -1).section('(', 0, 0)).fileName())
                           .arg(addrStr)
                           .arg(srcFile)
                           .arg(srcLine);
        stackLines << line;
        // 显示原始栈帧
        stackLines << QString("       原始信息：%1").arg(rawFrame);
    }

    free(symbols);
    return stackLines.join("\n");
}

namespace SignalDebugHelper
{
    /**
     * @brief 获取发送者的所有信号
     */
    inline QList<QMetaMethod> getSenderSignals(const QMetaObject *senderMeta)
    {
        QList<QMetaMethod> signalList;
        if (!senderMeta)
            return signalList;

        // 遍历所有方法，筛选出信号类型
        for (int i = 0; i < senderMeta->methodCount(); ++i)
        {
            QMetaMethod method = senderMeta->method(i);
            if (method.methodType() == QMetaMethod::Signal)
            {  // 筛选信号类型
                signalList.append(method);
            }
        }
        return signalList;
    }

    /**
     * @brief 解析信号发送者的元信息
     */
    inline void
        getSenderMetaInfo(QObject *sender, int signalIndex, QString &signalName, QString &signalSig, int &paramCount)
    {
        if (!sender)
            return;

        const QMetaObject *senderMeta = sender->metaObject();
        signalName                    = "未知信号";
        signalSig                     = "未知签名";
        paramCount                    = 0;

        QList<QMetaMethod> signalList = getSenderSignals(senderMeta);
        for (const QMetaMethod &sig : signalList)
        {
            // 获取信号的绝对索引
            int sigAbsoluteIndex = senderMeta->indexOfMethod(sig.methodSignature());
            if (sigAbsoluteIndex == signalIndex)
            {
                // 匹配成功：解析信号信息
                signalName = QString::fromUtf8(sig.name());
                signalSig  = QString::fromUtf8(sig.methodSignature());
                paramCount = sig.parameterCount();
                return;
            }
        }

        // ========== 降级逻辑：兼容普通方法解析==========
        int realIndex = signalIndex - senderMeta->methodOffset();

        if (realIndex < 0)
        {
            signalName = "内置信号（非自定义）";
            signalSig  = "内置信号（非自定义）";
        }
        else if (realIndex < senderMeta->methodCount())
        {
            QMetaMethod method = senderMeta->method(realIndex);
            signalName         = QString::fromUtf8(method.name());
            signalSig          = QString::fromUtf8(method.methodSignature());
            paramCount         = method.parameterCount();
        }
        else
        {
            signalName = "未知信号（索引越界）";
            signalSig  = "未知信号（索引越界）";
        }
    }

    /**
     * @brief 核心打印函数
     */
    inline void printSignalDetailedSource(QObject *receiver, QObject *signalSender, int signalIndex, int stackDepth)
    {
        if (!receiver || !signalSender)
        {
            qWarning("[SignalDebugger] 无效参数：槽函数对象/信号发送者为空");
            return;
        }

        const QMetaObject *senderMeta = signalSender->metaObject();
        // 过滤所有 destroyed 信号重载

        // 获取信号元信息
        QString signalName = "未知信号";
        QString signalSig  = "未知签名";
        int     paramCount = 0;
        getSenderMetaInfo(signalSender, signalIndex, signalName, signalSig, paramCount);

        // 打印完整信息
        qDebug() << "\n========================================";
        qDebug() << "[信号来源核心信息]";

        qDebug() << "  1. 信号发送者：";
        qDebug() << "     - 地址：" << signalSender;
        qDebug() << "     - 类名：" << senderMeta->className();
        qDebug() << "     - 对象名：" << (signalSender->objectName().isEmpty() ? "未命名" : signalSender->objectName());

        qDebug() << "  2. 信号信息：";
        qDebug() << "     - 名称：" << signalName;
        qDebug() << "     - 完整签名：" << signalSig;
        qDebug() << "     - 参数个数：" << paramCount;

        qDebug() << "  3. 槽函数信息：";
        qDebug() << "     - 所属对象：" << receiver;
        qDebug() << "     - 所属类名：" << receiver->metaObject()->className();

        // 修复换行问题：使用 qDebug().noquote() 避免转义 \n
        QString stackTrace = parseLinuxCallStack(stackDepth);
        qDebug().noquote() << "[调用栈]：\n" << stackTrace;
        qDebug() << "========================================\n";
    }

    /**
     * @brief 槽函数前置校验（避免析构阶段误触发）
     */
    inline bool checkSlotValid(QObject *receiver, QObject *sender)
    {
        if (!receiver)
        {
            qWarning("[SlotGuard] 槽函数所属对象为空");
            return false;
        }
        if (receiver->thread() && receiver->thread()->isFinished())
        {
            qWarning("[SlotGuard] 对象所属线程已结束");
            return false;
        }
        if (!sender)
        {
            qWarning("[SlotGuard] 槽函数非信号触发（手动调用）");
            return false;
        }
        return true;
    }
}  // namespace SignalDebugHelper

// 编译期判断是否为整数类型（包含所有整数类型：int、long、uint等）
template<typename T>
constexpr bool is_integer_type()
{
    return std::is_integral<T>::value && !std::is_same<T, bool>::value;  // 排除bool
}

// 编译期检查数值是否为非负整数
constexpr bool is_non_negative_integer(std::intmax_t value)
{
    return value >= 0;
}

// 强制参数为编译期整数常量
template<std::intmax_t N>
struct CompileTimeStackDepthChecker
{
    static constexpr int value = static_cast<int>(N);
};

// 将参数转为编译期常量
#define __SDH_NUM_CHECK(x) CompileTimeStackDepthChecker<static_cast<std::intmax_t>(x)>::value

// 1. 无参数宏（默认stackDepth=15）
#define PRINT_SIGNAL_DETAILED_SOURCE()                                                                      \
    do                                                                                                      \
    {                                                                                                       \
        constexpr int __stack_depth   = 15;                                                                 \
        QObject      *__signal_sender = this->sender();                                                     \
        if (!SignalDebugHelper::checkSlotValid(this, __signal_sender))                                      \
        {                                                                                                   \
            break;                                                                                          \
        }                                                                                                   \
        int __signal_index = this->senderSignalIndex();                                                     \
        SignalDebugHelper::printSignalDetailedSource(this, __signal_sender, __signal_index, __stack_depth); \
    } while (0)

// 2. 带参数宏,指定调用栈层数
#define PRINT_SIGNAL_DETAILED_SOURCE_WITH_DEPTH(stackDepth)                                                  \
    do                                                                                                       \
    {                                                                                                        \
        constexpr int __stack_depth = __SDH_NUM_CHECK(stackDepth);                                           \
        static_assert(is_non_negative_integer(__stack_depth), "stackDepth must be a non-negative integer!"); \
        QObject *__signal_sender = this->sender();                                                           \
        if (!SignalDebugHelper::checkSlotValid(this, __signal_sender))                                       \
        {                                                                                                    \
            break;                                                                                           \
        }                                                                                                    \
        int __signal_index = this->senderSignalIndex();                                                      \
        SignalDebugHelper::printSignalDetailedSource(this, __signal_sender, __signal_index, __stack_depth);  \
    } while (0)
