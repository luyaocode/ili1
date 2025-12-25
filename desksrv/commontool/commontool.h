#ifndef COMMONTOOL_H
#define COMMONTOOL_H

#include <QWidget>
#include <QProcess>
#include <QString>
#include <cmath>
#include "commontool_global.h"

class QTime;
class COMMONTOOLSHARED_EXPORT Commontool
{

public:
    // 设置 X11 窗口的 override_redirect 属性
    // 开启后,alt+tab 无法切换窗口
    static void setOverrideRedirect(QWidget *window, bool enable);

    static void moveToBottomLeft(QWidget *window);

    // 获取配置文件路径（Linux 优先使用 XDG 规范路径）
    static QString getConfigFilePath(const QString &appName, const QString &defaultCfgName);
    // 获取持久化文件路径
    static QString getWritablePath(const QString& appName,const QString& dirName,const QString& defaultPath);

    // 命令行启动程序
    static void executeThirdProcess(const QString &cmd, const QStringList &params = {});

    // 路径转换
    static QString formatPath(const QString& path);
    // 检查路径是否存在
    static bool checkPathExist(const QString &path);

    // 第三方工具打开本地URL
    static bool openLocalUrl(const QString& path);

    // 打开终端并cd到目录
    static bool openTerminal(const QString& path);

    /**
     * @brief 字节数转换为带单位的字符串（自适应 B/KB/MB/GB/TB）
     * @param bytes 原始字节数（支持 unsigned long long，避免大文件溢出）
     * @param precision 小数精度（默认 1 位，如 1.2 MB）
     * @return 格式化后的字符串（如 "1.5 KB"、"2.3 GB"）
     */
    static QString formatFileSize(unsigned long long bytes, int precision = 1);

    static bool isTimeListsContentEqual(const QList<QTime> &list1, const QList<QTime> &list2);
};

#endif  // COMMONTOOL_H
