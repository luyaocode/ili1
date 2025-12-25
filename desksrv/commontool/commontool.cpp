#include "commontool.h"
#include <QDebug>
#include <QThread>
#include <QTime>
#include <QMutex>
#include <QMutexLocker>
#include <QGuiApplication>
#include <QScreen>
#include <QDir>
#include <QUrl>
#include <QDesktopServices>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QProcess>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
void Commontool::setOverrideRedirect(QWidget *window, bool enable)
{
    if (!window)
    {
        return;
    }
    WId wid = window->winId();
    if (wid == 0)
    {
        return;
    }

    Display *display = XOpenDisplay(nullptr);
    if (!display)
    {
        return;
    }

    XSetWindowAttributes attrs;
    attrs.override_redirect = enable ? True : False;
    XChangeWindowAttributes(display, static_cast<Window>(wid), CWOverrideRedirect, &attrs);

    XFlush(display);
    XCloseDisplay(display);
}

void Commontool::moveToBottomLeft(QWidget *window)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;
    QRect screenRect = screen->geometry();  // 使用整个屏幕区域，包括任务栏

    // 计算位置
    int x = screenRect.left();
    int y = screenRect.bottom() - window->height();
    window->move(x, y);
}

QString Commontool::getConfigFilePath(const QString &appName, const QString &defaultCfgName)
{
    if (appName.isEmpty() || defaultCfgName.isEmpty())
    {
        return QString();
    }

    // XDG 配置目录：优先使用环境变量 XDG_CONFIG_HOME，默认 ~/.config
    QString xdgConfigHome = qgetenv("XDG_CONFIG_HOME");
    if (xdgConfigHome.isEmpty())
    {
        xdgConfigHome = QDir::homePath() + "/.config";
    }

    // 应用专属目录：~/.config/QuickManager
    QString appConfigDir = xdgConfigHome + "/" + appName;
    QDir    dir(appConfigDir);
    // 若目录不存在，创建（包括父目录）
    if (!dir.exists())
    {
        if (!dir.mkpath("."))
        {  // mkpath 递归创建目录
            qWarning() << "Failed to create config directory:" << appConfigDir;
            return defaultCfgName;  // 创建失败时返回默认文件
        }
    }

    // 最终配置文件路径
    return appConfigDir + "/" + defaultCfgName;
}

QString Commontool::getWritablePath(const QString &appName, const QString &dirName, const QString &defaultPath)
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + appName + "/" + dirName;
    bool    isDirValid = false;
    QDir    dir(dataDir);
    if (dir.mkpath("."))
    {
        isDirValid = true;
    }

    if (!isDirValid)
    {
        if (!defaultPath.isEmpty())
        {
            dataDir = defaultPath;
            QDir dir(dataDir);
            if (dir.mkpath("."))
            {
                isDirValid = true;
            }
        }
        if (!isDirValid)
        {
            dataDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
            if (dataDir.isEmpty() || !QDir().mkpath(dataDir))
            {
                dataDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
            }
            dataDir = QDir(dataDir).filePath(appName + "/" + dirName);
            QDir().mkpath(dataDir);
        }
    }

    return dataDir;
}

void Commontool::executeThirdProcess(const QString &cmd, const QStringList &params)
{
    if (cmd.isEmpty())
    {
        return;
    }
    auto formatedPath = formatPath(cmd);
    // 3. 创建堆上 QProcess（无父对象，通过信号槽自动释放）
    QProcess *process = new QProcess();

    // Linux 优化：合并 stdout/stderr，避免日志分散；禁用 shell 转发（减少开销，避免终端窗口）
    process->setProcessChannelMode(QProcess::MergedChannels);

    // 4. 连接进程结束信号：释放内存 + 输出日志
    QObject::connect(
        process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),  // 显式指定重载
        process,
        [=](int exitCode, QProcess::ExitStatus exitStatus) {
            const QString status = (exitStatus == QProcess::NormalExit) ? "Normal" : "Crashed";
            qInfo() << "[CommonTool] Async process finished [Linux]. Cmd:" << formatedPath << ", ExitCode:" << exitCode
                    << ", Status:" << status;

            QByteArray output = process->readAll();
            if (!output.isEmpty())
            {
                qInfo() << "[CommonTool] Process output:\n" << QString::fromUtf8(output);
            }
            process->kill();
            process->waitForFinished(1000);  // 等待进程终止
            process->deleteLater();
        },
        Qt::QueuedConnection);

    // 5. 连接进程错误信号：处理启动/运行失败
    QObject::connect(
        process, &QProcess::errorOccurred, process,
        [=](QProcess::ProcessError error) {
            Q_UNUSED(error)
            qCritical() << "[CommonTool] Async process error [Linux]. Cmd:" << formatedPath
                        << ", Error:" << process->errorString();

            // 出错后立即释放内存
            process->kill();
            process->waitForFinished(1000);  // 等待进程终止
            process->deleteLater();
        },
        Qt::QueuedConnection);

    // 6. 异步启动进程（无阻塞，函数直接返回，进程后台运行）
    qInfo() << "[CommonTool] Start async process [Linux]. Cmd:" << formatedPath << ", Params:" << params;
    process->start(formatedPath, params);

    // 7. 快速校验启动状态（1秒超时，避免启动失败后残留僵尸进程）
    if (!process->waitForStarted(1000))
    {
        qCritical() << "[CommonTool] Process start failed [Linux]. Cmd:" << formatedPath
                    << ", Error:" << process->errorString();
        // 启动超时：先终止进程再释放，避免僵尸进程
        process->kill();
        process->waitForFinished(1000);  // 等待进程终止
        process->deleteLater();
    }
}

/**
 * @brief 扩展波浪号（~ 或 ~username）
 * @param path 输入路径，会被原地修改
 */
static auto expandTilde = [](QString &path) {
    if (!path.startsWith("~"))
        return;

    int     slashIndex = path.indexOf('/');
    QString username, expandedPath;

    if (slashIndex == 1)
    {
        // 格式：~/subdir → 当前用户主目录
        expandedPath = QDir::homePath() + path.mid(1);
    }
    else if (slashIndex > 1)
    {
        // 格式：~username/subdir → 其他用户主目录
        username = path.mid(1, slashIndex - 1);
        // 调用系统命令获取其他用户主目录（Linux/macOS 适用）
        QProcess process;
        process.start("getent", {"passwd", username});
        process.waitForFinished(1000);  // 1秒超时
        QString output = process.readAllStandardOutput();
        if (!output.isEmpty())
        {
            QStringList parts = output.split(':');
            if (parts.size() >= 6)
            {
                expandedPath = parts[5] + path.mid(slashIndex);
            }
        }
        // 若获取失败，保持原样并警告
        if (expandedPath.isEmpty())
        {
            qDebug() << "[Commontool] Warning: Failed to expand ~" << username;
            return;
        }
    }
    else
    {
        // 格式：~ → 当前用户主目录
        expandedPath = QDir::homePath();
    }

    path = expandedPath;
};

/**
 * @brief 扩展环境变量（$VAR 或 ${VAR} 格式）
 * @param path 输入路径，会被原地修改
 */
static auto expandEnvironmentVariables = [](QString &path) {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    // 正则表达式：匹配 $VAR 或 ${VAR}
    QRegularExpression              re(R"(\$(\w+|\{([^}]+)\}))");
    QRegularExpressionMatchIterator it = re.globalMatch(path);

    // 从后往前替换，避免替换后影响后续匹配位置
    QList<QPair<int, int>> matches;  // <位置, 长度>
    QList<QString>         replacements;
    while (it.hasNext())
    {
        QRegularExpressionMatch match = it.next();
        matches.prepend({match.capturedStart(), match.capturedLength()});

        QString varName;
        if (match.captured(0).startsWith("${"))
        {
            varName = match.captured(2);  // ${VAR} → VAR
        }
        else
        {
            varName = match.captured(1);  // $VAR → VAR
        }

        QString varValue = env.value(varName);
        replacements.prepend(varValue);
    }

    // 执行替换
    for (int i = 0; i < matches.size(); ++i)
    {
        path.replace(matches[i].first, matches[i].second, replacements[i]);
    }
};

/**
 * @brief 规范化路径（处理 .、..、相对路径转绝对路径）
 * @param path 输入路径，会被原地修改
 */
static auto normalizePath = [](QString &path) {
    // 若为相对路径，转换为绝对路径（基于当前工作目录）
    if (!QDir::isAbsolutePath(path))
    {
        path = QDir::current().absoluteFilePath(path);
    }
    // 规范化：解析 .、..，合并重复分隔符，统一为系统分隔符
    path = QDir::cleanPath(path);
};

/**
 * @brief 规范化路径（处理 .、..、相对路径转绝对路径）
 * @param path 输入路径，会被原地修改
 */
QString Commontool::formatPath(const QString &path)
{
    if (path.isEmpty())
    {
        return {};
    }
    QString formatedPath = path;
    // 1. 扩展波浪号
    expandTilde(formatedPath);

    // 2. 扩展环境变量
    expandEnvironmentVariables(formatedPath);

    // 3. 规范化路径
    normalizePath(formatedPath);
    return formatedPath;
}

bool Commontool::checkPathExist(const QString &path)
{
    QString   formatedPath = formatPath(path);
    QString   subPath      = formatedPath.left(path.indexOf(' '));
    QFileInfo fileInfo(subPath);
    return fileInfo.exists();
}

bool Commontool::openLocalUrl(const QString &path)
{
    QString formatedPath = formatPath(path);
    if (!checkPathExist(formatedPath))
    {
        QFileInfo fileInfo(formatedPath);
        formatedPath = fileInfo.absolutePath();
        qDebug() << "[Commontool] Error: Path not exist:" << formatedPath;
        return false;
    }
    // 转换为 QUrl 并打开
    QUrl dirUrl = QUrl::fromLocalFile(formatedPath);
    if (!dirUrl.isValid())
    {
        qDebug() << "[Commontool] Error: Invalid URL:" << dirUrl.toString();
        return false;
    }

    bool success = QDesktopServices::openUrl(dirUrl);
    if (!success)
    {
        qDebug() << "[Commontool] Error: Failed to open directory:" << formatedPath;
        return false;
    }
    qDebug() << "[Commontool] Open directory:" << formatedPath;
    return success;
}

bool Commontool::openTerminal(const QString &path)
{
    QString formatedPath = formatPath(path);
    if (!checkPathExist(formatedPath))
    {
        QFileInfo fileInfo(formatedPath);
        formatedPath = fileInfo.absolutePath();
        if (!checkPathExist(formatedPath))
        {
            qDebug() << "[Commontool] Error: Path not exist:" << formatedPath;
            return false;
        }
    }

    // 自动检测系统终端（优先使用用户默认终端）
    QString terminal = QStandardPaths::findExecutable("x-terminal-emulator");
    if (terminal.isEmpty())
    {
        // 回退方案：尝试常见终端
        QStringList terminals = {"gnome-terminal", "konsole", "xfce4-terminal", "lxterminal"};
        for (const QString &term : terminals)
        {
            if (QStandardPaths::findExecutable(term).isEmpty())
            {
                terminal = term;
                break;
            }
        }
    }

    if (terminal.isEmpty())
    {
        qDebug() << "[Commontool] No terminal found!";
        return false;
    }

    QStringList args;
    if (terminal.contains("gnome-terminal") || terminal.contains("xfce4-terminal"))
    {
        args << "--working-directory=" + formatedPath;
    }
    else if (terminal.contains("konsole"))
    {
        args << "--workdir" << formatedPath;
    }
    else
    {
        // 通用方案：使用 bash -c
        terminal = "bash";
        args << "-c"
             << QString("cd %1 && %2").arg(formatedPath).arg(QStandardPaths::findExecutable("x-terminal-emulator"));
    }

    qDebug() << "[Commontool] Opening terminal:" << terminal << args.join(" ");
    bool success = QProcess::startDetached(terminal, args);
    if (!success)
    {
        qDebug() << "[Commontool] Failed to open terminal!";
    }
    return true;
}

QString Commontool::formatFileSize(unsigned long long bytes, int precision)
{
    // 单位数组（从 B 到 TB）
    const QStringList units = {"B", "KB", "MB", "GB", "TB"};
    // 单位换算系数（1 KB = 1024 B）
    const double unitStep = 1024.0;

    // 特殊处理：0 字节直接返回 "0 B"
    if (bytes == 0)
    {
        return QString("0 %1").arg(units[0]);
    }

    // 计算当前字节数对应的单位索引（log2(bytes) / 10，因为 2^10 = 1024）
    int unitIndex = qMin(static_cast<int>(log(static_cast<double>(bytes)) / log(unitStep)), units.size() - 1);

    // 计算转换后的数值（如 2048 B → 2.0 KB）
    double convertedValue = static_cast<double>(bytes) / pow(unitStep, unitIndex);

    // 格式化字符串（根据精度保留小数，避免整数显示 .0 后缀）
    if (precision == 0 || convertedValue == floor(convertedValue))
    {
        // 无小数或整数场景，显示为整数（如 2 KB 而非 2.0 KB）
        return QString("%1 %2").arg(static_cast<qint64>(convertedValue)).arg(units[unitIndex]);
    }
    else
    {
        // 保留指定精度的小数（如 1.23 MB）
        return QString("%1 %2").arg(convertedValue, 0, 'f', precision).arg(units[unitIndex]);
    }
}

bool Commontool::isTimeListsContentEqual(const QList<QTime> &list1, const QList<QTime> &list2)
{
    if (list1.size() != list2.size())
    {
        return false;
    }
    QList<QTime> sortedList1 = list1;
    QList<QTime> sortedList2 = list2;
    std::sort(sortedList1.begin(), sortedList1.end());
    std::sort(sortedList2.begin(), sortedList2.end());

    return sortedList1 == sortedList2;
}
