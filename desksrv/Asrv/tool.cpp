#include "tool.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QUrl>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkInterface>
#include <QProcess>
#include <QWebSocket>
#include <QDebug>
#include <QRegExp>
#include <QByteArray>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QByteArray>
#include <termios.h>
#include <pty.h>
#include <unistd.h>
#include <sys/wait.h>

#include "unicode.h"

QByteArray readTemplate(const QString &templateName, const QMap<QString, QString> &variables)
{
    // 模板文件路径（放在www目录下）
    QString templatePath = QCoreApplication::applicationDirPath() + "/www/" + templateName;
    QFile   templateFile(templatePath);

    // 模板文件不存在时返回默认内容
    if (!templateFile.open(QIODevice::ReadOnly))
    {
        qWarning() << "模板文件不存在：" << templatePath;
        return "模板文件加载失败：" + templateName.toUtf8();
    }

    // 读取模板内容
    QByteArray templateContent = templateFile.readAll();
    templateFile.close();

    // 替换所有占位符
    QString contentStr = QString(templateContent);
    for (auto it = variables.constBegin(); it != variables.constEnd(); ++it)
    {
        contentStr.replace("{{" + it.key() + "}}", it.value());
    }

    return contentStr.toUtf8();
}

QByteArray generateDirListHtml(const QString &dirPath, const QString &rootDir, const QString &requestPath)
{
    QDir dir(dirPath);
    if (!dir.exists())
    {
        // 使用404模板
        QMap<QString, QString> vars;
        vars["ERROR_MSG"] = "请求的目录不存在：" + dirPath;
        return readTemplate("404.html", vars);
    }

    // 读取目录文件列表
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);
    dir.setSorting(QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    QFileInfoList fileList = dir.entryInfoList();

    // 1. 构建上级目录HTML片段
    QString parentDirHtml = "";
    if (dirPath != rootDir)
    {
        QString parentRequestPath =
            requestPath.endsWith("/") ? requestPath.left(requestPath.length() - 1) : requestPath;
        parentRequestPath =
            parentRequestPath.lastIndexOf("/") > 0 ? parentRequestPath.left(parentRequestPath.lastIndexOf("/")) : "/";
        QString icon  = DIR_ICON;
        parentDirHtml = QString(R"(
                                <li class="file-item" >
                                <span class="dir-icon">%1</span>
                                <a href="%2">../ (上级目录)</a>
                                </li>
                                )")
                            .arg(icon)
                            .arg(parentRequestPath);
    }

    // 2. 构建文件列表HTML片段
    QString fileListHtml = "";
    for (const QFileInfo &fileInfo : fileList)
    {
        bool    isDir    = fileInfo.isDir();
        QString fileName = fileInfo.fileName();
        QString fileUrl  = requestPath.endsWith("/") ? requestPath + fileName : requestPath + "/" + fileName;
        QString fileSize = fileInfo.isDir() ? "目录" : QString("%1 KB").arg(fileInfo.size() / 1024);
        // 新增：获取并格式化修改时间（yyyy-MM-dd HH:mm:ss 格式，可按需调整）
        QString modifyTime   = fileInfo.lastModified().toString("yyyy-MM-dd HH:mm:ss");
        QString icon         = isDir ? DIR_ICON : FILE_ICON;
        QString diritem      = isDir ? "dir-item" : "";
        QString strFilePath  = "/" + QDir(rootDir).relativeFilePath(fileInfo.absoluteFilePath());
        QString datafilepath = isDir ? "" : QString("data-file-path=\"%1\"").arg(strFilePath);
        QString iconclass    = isDir ? "dir" : "file";
        QString downloadBtn =
            isDir ? "" : QString(R"(<button class="download-btn" data-file-path="%1">下载</button>)").arg(strFilePath);

        // 修改：在HTML中插入修改时间<span class="file-modify-time">%7</span>，调整占位符顺序
        fileListHtml += QString(R"(
                                <li class="file-item %1" %2>
                                <span class="%3-icon">%4</span>
                                <a href="%5">%6</a>
                                <span class="file-modify-time">%7</span>
                                <span class="file-size">%8</span>
                                %9
                                </li>
                                )")
                            .arg(diritem)
                            .arg(datafilepath)
                            .arg(iconclass)
                            .arg(icon)
                            .arg(fileUrl)
                            .arg(fileName)
                            .arg(modifyTime)    // 第7个占位符：修改时间
                            .arg(fileSize)      // 第8个占位符：文件大小
                            .arg(downloadBtn);  // 第9个占位符：下载按钮
    }

    // 3. 定义模板变量
    QMap<QString, QString> variables;
    variables["DIR_PATH"]     = dirPath;        // 服务端实际路径
    variables["REQUEST_PATH"] = requestPath;    // HTTP请求路径
    variables["PARENT_DIR"]   = parentDirHtml;  // 上级目录HTML
    variables["FILE_LIST"]    = fileListHtml;   // 文件列表HTML
                                                //    variables["JS_PATH"] = getStaticPath(EM_DIR::JS); // js文件

    // 4. 读取模板并替换变量
    return readTemplate("dir_list.html", variables);
}

bool isValidPath(const QString &filePath, const QString &rootDir)
{
    // 1. 获取映射根目录的绝对路径（m_rootDir 是你配置的文件系统根目录，如 "/home/chenluyao" 或 "/"）
    QDir    root(rootDir);
    QString rootAbsPath = root.absolutePath();
    // 补充：确保根目录路径以路径分隔符结尾（避免误判，如 "/home/chen" 匹配 "/home/chen123"）
    if (!rootAbsPath.endsWith(QDir::separator()))
    {
        rootAbsPath += QDir::separator();
    }

    // 2. 获取输入路径的绝对路径（消除../、./等相对路径干扰）
    QFileInfo fileInfo(filePath);
    QString   fileAbsPath = fileInfo.absoluteFilePath();
    // 补充：若输入路径是目录，也添加路径分隔符（统一判断规则）
    if (fileInfo.isDir() && !fileAbsPath.endsWith(QDir::separator()))
    {
        fileAbsPath += QDir::separator();
    }

    // 3. 核心校验逻辑
    bool isSubPath = fileAbsPath.startsWith(rootAbsPath);  // 是否是映射根目录的子路径
    bool isExists  = fileInfo.exists();                    // 路径是否真实存在（可选，建议保留）

    // 调试打印（方便定位问题）
    qDebug() << "映射根目录绝对路径：" << rootAbsPath;
    qDebug() << "输入路径绝对路径：" << fileAbsPath;
    qDebug() << "是否为子路径：" << isSubPath << "，路径是否存在：" << isExists;

    // 最终返回：必须是子路径 + 路径存在（可根据需求调整，如仅保留 isSubPath）
    return isSubPath && isExists;
}

QString getMimeType(const QString &filePath)
{
    // 1. 若为目录，直接返回 HTML 类型（目录列表是 HTML 页面）
    QFileInfo fileInfo(filePath);
    if (fileInfo.isDir())
    {
        return "text/html; charset=UTF-8";
    }

    // 2. 提取文件后缀（转小写，兼容大小写后缀如 .HTML/.Js）
    QString suffix = fileInfo.suffix().toLower();

    // 3. 从全局常量中获取，兜底返回二进制流类型
    return MIME_MAP.value(suffix, "application/octet-stream");
}

QByteArray readFileOrDir(const QString &filePath, const QString &rootDir, const QString &requestPath, bool isPreview)
{
    if (!isValidPath(filePath, rootDir))
    {
        qWarning() << "非法路径访问：" << filePath;
        QMap<QString, QString> vars;
        vars["ERROR_MSG"] = "非法文件路径或无访问权限：" + filePath;
        return readTemplate("403.html", vars);
    }

    QFileInfo fileInfo(filePath);
    // 目录：生成目录列表（基于模板）
    if (fileInfo.isDir())
    {
        return generateDirListHtml(filePath, rootDir, requestPath);
    }

    if (isPreview)
    {
        // 判断文件大小是否超过阈值
        qint64                 fileSize = fileInfo.size();
        QMap<QString, QString> vars;
        if (fileSize > MAX_PREVIEW_SIZE)
        {
            vars["ERROR_MSG"] = QString("文件过大，不支持预览！文件大小：%1，最大允许预览大小：%2")
                                    .arg(formatFileSize(fileSize))
                                    .arg(formatFileSize(MAX_PREVIEW_SIZE));
        }
        QString mimeType = getMimeType(filePath);
        if (mimeType == "application/octet-stream")
        {
            QMap<QString, QString> vars;
            vars["ERROR_MSG"] = QString("文件类型不支持预览！");
        }
        if (vars.size() > 0)
        {
            return readTemplate("500.html", vars);
        }
    }

    // 文件：读取文件内容
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "无法打开文件：" << filePath << "，错误：" << file.errorString();
        QMap<QString, QString> vars;
        vars["ERROR_MSG"] = "请求的文件不存在或无访问权限：" + filePath;
        return readTemplate("404.html", vars);
    }

    QByteArray content = file.readAll();
    file.close();
    return content;
}

QString formatFileSize(qint64 bytes)
{
    if (bytes < 1024)
        return QString("%1 B").arg(bytes);
    if (bytes < 1024 * 1024)
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    if (bytes < 1024 * 1024 * 1024)
        return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 1);
    return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 1);
}

QString decodeFilePath(const QString &encodedPath)
{
    // 1. 将 QString 转换为 QByteArray（UTF-8 格式）
    QByteArray encodedBytes = encodedPath.toUtf8();
    // 2. 进行百分号解码
    return QUrl::fromPercentEncoding(encodedBytes);
}

QByteArray parseChunkedData(const QByteArray &data)
{
    QByteArray body;
    int        pos = data.indexOf("\r\n\r\n") + 4;  // 跳过请求头
    if (pos < 4)
        return body;

    QByteArray chunkData = data.mid(pos);
    while (!chunkData.isEmpty())
    {
        // 提取块大小（十六进制）
        int chunkSizeEnd = chunkData.indexOf("\r\n");
        if (chunkSizeEnd == -1)
            break;
        QByteArray chunkSizeStr = chunkData.left(chunkSizeEnd).trimmed();
        bool       ok;
        int        chunkSize = chunkSizeStr.toInt(&ok, 16);
        if (!ok || chunkSize == 0)
            break;  // 块大小为0表示结束

        // 提取块数据
        int        chunkDataStart = chunkSizeEnd + 2;
        QByteArray chunk          = chunkData.mid(chunkDataStart, chunkSize);
        body.append(chunk);

        // 移动到下一个块（跳过当前块的\r\n）
        chunkData = chunkData.mid(chunkDataStart + chunkSize + 2);
    }

    return body;
}

void sendErrorResponse(QTcpSocket *socket, int statusCode, const QString &message)
{
    QJsonObject responseJson;
    responseJson["success"] = false;
    responseJson["message"] = message;

    sendJsonResponse(socket, statusCode, message);
}

void sendJsonResponse(QTcpSocket *socket, int statusCode, const QString &message)
{
    // 构建响应体（先构建，再计算长度）
    QJsonObject responseJson;
    responseJson["success"] = (statusCode == 200);
    responseJson["message"] = message;
    QByteArray responseBody = QJsonDocument(responseJson).toJson(QJsonDocument::Compact);
    qDebug().noquote() << "Response: " << QString::fromUtf8(responseBody);
    QByteArray response;
    // 构建状态行
    if (statusCode == 200)
    {
        response += "HTTP/1.1 200 OK\r\n";
    }
    else if (statusCode == 400)
    {
        response += "HTTP/1.1 400 Bad Request\r\n";
    }
    else if (statusCode == 500)
    {
        response += "HTTP/1.1 500 Internal Server Error\r\n";
    }

    // 构建响应头（关键：Content-Length 为响应体的字节数）
    response += "Content-Type: application/json; charset=UTF-8\r\n";
    response += "Connection: close\r\n";
    response += "Content-Length: " + QByteArray::number(responseBody.size()) + "\r\n\r\n";

    // 拼接响应体
    response += responseBody;

    // 发送响应
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

// 适配 Qt 5.9：仅获取第一个有效本地 IPv4 地址（排除回环）
QString getLocalIpv4()
{
    // 遍历所有网络接口
    foreach (const QNetworkInterface &iface, QNetworkInterface::allInterfaces())
    {
        // 关键修改：Qt 5.9 用 flags() 判断回环接口（替代 isLoopBack()）
        // 过滤条件：启用（IsUp）、运行中（IsRunning）、非回环（!IsLoopBack）
        if (iface.flags() & QNetworkInterface::IsUp && iface.flags() & QNetworkInterface::IsRunning &&
            !(iface.flags() & QNetworkInterface::IsLoopBack))
        {  // Qt 5.9 兼容写法

            // 遍历当前接口的所有地址条目
            foreach (const QNetworkAddressEntry &entry, iface.addressEntries())
            {
                QHostAddress ip = entry.ip();
                // 只保留 IPv4 地址
                if (ip.protocol() == QAbstractSocket::IPv4Protocol)
                {
                    // 找到第一个有效 IPv4，直接返回（确保只返回一个）
                    return ip.toString();
                }
            }
        }
    }
    // 无有效 IPv4 时返回空字符串
    return "";
}

QString ansiToHtml(const QByteArray &input)
{
    QByteArray html = input;

    // 1. 转义 HTML 特殊字符（避免 XSS 和显示错乱）
    html = html.replace("&", "&amp;")
               .replace("<", "&lt;")
               .replace(">", "&gt;")
               .replace("\"", "&quot;")
               .replace("'", "&#39;")
               .replace("\n", "<br>")                       // 换行转 <br>
               .replace("\t", "&nbsp;&nbsp;&nbsp;&nbsp;");  // 制表符转 4 个空格

    // 2. ANSI 颜色映射表（常用前景色/背景色）
    QMap<QString, QString> colorMap = {
        // 前景色（文字颜色）
        {"30", "color: #000000;"},  // 黑色
        {"31", "color: #FF0000;"},  // 红色
        {"32", "color: #00FF00;"},  // 绿色
        {"33", "color: #FFFF00;"},  // 黄色
        {"34", "color: #0000FF;"},  // 蓝色
        {"35", "color: #FF00FF;"},  // 紫色
        {"36", "color: #00FFFF;"},  // 青色
        {"37", "color: #FFFFFF;"},  // 白色
        // 背景色
        {"40", "background-color: #000000;"},
        {"41", "background-color: #FF0000;"},
        {"42", "background-color: #00FF00;"},
        {"43", "background-color: #FFFF00;"},
        {"44", "background-color: #0000FF;"},
        {"45", "background-color: #FF00FF;"},
        {"46", "background-color: #00FFFF;"},
        {"47", "background-color: #FFFFFF;"},
        // 样式
        {"0", ""},                            // 重置
        {"1", "font-weight: bold;"},          // 加粗
        {"4", "text-decoration: underline;"}  // 下划线
    };

    // 3. 匹配 ANSI 转义序列（格式：\x1B[数字;数字;...m）
    QRegularExpression              ansiRegex(R"(\x1B\[([0-9;]+)m)");
    QRegularExpressionMatchIterator iter = ansiRegex.globalMatch(html);

    // 4. 替换 ANSI 序列为 HTML <span> 样式
    while (iter.hasNext())
    {
        QRegularExpressionMatch match    = iter.next();
        QString                 ansiCode = match.captured(1);  // 提取数字部分（如 "01;34"）
        QString                 style    = "";

        // 解析多个 ANSI 代码（用 ; 分隔）
        QStringList codes = ansiCode.split(";", QString::SkipEmptyParts);
        for (const QString &code : codes)
        {
            if (colorMap.contains(code))
            {
                style += colorMap[code];
            }
        }

        if (style.isEmpty())
        {
            // 重置样式（</span> 闭合之前的标签）
            html.replace(match.capturedStart(), match.capturedLength(), "</span>");
        }
        else
        {
            // 开启新样式（<span style="...">）
            // 修正后代码（添加 toUtf8() 转换）
            html.replace(match.capturedStart(), match.capturedLength(),
                         QString("<span style=\"%1\">").arg(style).toUtf8());  // 转为 QByteArray
        }
    }

    // 5. 确保所有 <span> 标签闭合（避免样式溢出）
    int openSpanCount = html.count("<span style=") - html.count("</span>");
    for (int i = 0; i < openSpanCount; ++i)
    {
        html += "</span>";
    }

    return QString(html);
}
// 实现过滤ANSI转义控制字符的函数
QByteArray filterAnsiEscapeCodes(const QByteArray &input)
{
    // 将QByteArray转换为QString，方便正则处理
    QString str = QString::fromUtf8(input);

    /**
        * 优化后的正则表达式，匹配所有ANSI转义控制码：
        * \x1B          : 匹配ESC字符（\033，ASCII码27）
        * (?:           : 非捕获组，用于分组匹配多种情况
        *   \[.*?m      : 匹配CSI序列（颜色控制码）：ESC[开头，任意字符（非贪婪），以m结尾（如[0m、[01;34m）
        *   | \].*?\x07 : 匹配OSC序列（窗口标题等）：ESC]开头，任意字符，以\07（BEL）结尾
        *   | [@-Z\\-_] : 匹配其他单字符ANSI转义码（如ESC[、ESC]之外的单字符控制码）
        * )
        */
    QRegularExpression ansiRegex(R"(\x1B(?:\[[\d;]*m|\][^\x07]*\x07|[@-Z\\-_]))");

    // 全局替换所有匹配的控制码为空字符串（关键：确保替换所有匹配项，包括连续的）
    str = str.replace(ansiRegex, "");

    // 转换回QByteArray返回
    return str.toUtf8();
}

void setTermAttr(struct termios &termAttr)
{
    // 配置 PTY 时添加 c_cflag 配置（在 tcgetattr 后、tcsetattr 前）
    termAttr.c_cflag = 0;
    termAttr.c_cflag |= CS8;     // 8 位数据位（标准终端配置）
    termAttr.c_cflag |= CREAD;   // 启用接收数据（必需，否则无法读取输入）
    termAttr.c_cflag |= CLOCAL;  // 忽略调制解调器状态线（本地终端）
    termAttr.c_cflag |= HUPCL;   // 关闭时挂断（可选，增强兼容性）
    // 设置波特率（默认 9600，兼容所有终端）
    cfsetispeed(&termAttr, B9600);
    cfsetospeed(&termAttr, B9600);
    // 关键属性：启用交互式终端必需功能
    // 重置 c_lflag 为基础值（避免继承父进程的异常配置）
    termAttr.c_lflag = 0;
    termAttr.c_lflag |= ECHO;    // 输入回显（用户能看到输入的密码/命令）
    termAttr.c_lflag |= ECHOE;   // 退格键（Backspace）回显（删除字符时终端更新）
    termAttr.c_lflag |= ECHOK;   // 执行 exit 等命令时回显确认
    termAttr.c_lflag |= ICANON;  // 规范模式（按回车提交输入，密码输入需此模式）
    termAttr.c_lflag |= ISIG;    // 支持信号（Ctrl+C 终止、Ctrl+D 退出）
    termAttr.c_lflag |= ICRNL;   // 回车→换行转换（确保输入被正确识别）
    termAttr.c_lflag |= BRKINT;  // 中断信号触发（终端状态正常）
    termAttr.c_lflag |= INPCK;   // 启用奇偶校验（可选，增强兼容性）
    termAttr.c_lflag |= ISTRIP;  // 剥离字符第 8 位（可选，兼容旧终端）

    // 输出配置
    termAttr.c_oflag = 0;
    termAttr.c_oflag |= ONLCR;  // 换行→回车+换行（避免终端显示错乱）
    termAttr.c_oflag |= OPOST;  // 启用输出处理（关键：确保密码提示正常输出）

    // 输入配置
    termAttr.c_iflag = 0;
    termAttr.c_iflag |= IGNBRK;  // 忽略中断条件
    termAttr.c_iflag |= ICRNL;   // 回车→换行（与 c_lflag 配合）
    termAttr.c_iflag |= INPCK;   // 启用输入校验
    termAttr.c_iflag |= ISTRIP;  // 剥离字符第 8 位

    // 控制字符配置（确保密码输入无超时、无最小字节限制）
    termAttr.c_cc[VMIN]  = 1;  // 最小读取 1 字节（实时响应输入）
    termAttr.c_cc[VTIME] = 0;  // 无超时（阻塞读取，直到用户输入）
}

void setTermAttrAsync(termios &termAttr)
{
    //    // 输入模式：保留核心功能，删除冗余选项
    //    termAttr.c_iflag = ICRNL | IGNBRK;  // 回车→换行 + 忽略中断条件
    //    // 输出模式：仅保留换行转换（避免显示错乱）
    //    termAttr.c_oflag = ONLCR | OPOST;
    //    // 本地模式：保留交互式必需功能
    //    termAttr.c_lflag = ECHO | ECHOE | ICANON | ISIG;  // 回显 + 规范模式 + 信号支持
    //    // 控制模式：基础配置（8位数据 + 启用接收 + 本地终端）
    //    termAttr.c_cflag = CS8 | CREAD | CLOCAL;

    //    // 波特率：使用系统默认（或显式设置 B9600，兼容所有终端）
    //    cfsetispeed(&termAttr, B9600);
    //    cfsetospeed(&termAttr, B9600);

    //    // 控制字符配置（适配非阻塞模式）
    //    termAttr.c_cc[VMIN]  = 0;
    //    termAttr.c_cc[VTIME] = 1;
    // 输入模式：回车→换行，忽略中断/奇偶校验，关闭流控（避免卡住）
    termAttr.c_iflag = ICRNL | IGNBRK | IGNPAR | IXOFF | IXON;
    // 输出模式：仅保留必要换行转换，关闭多余处理（避免终端错乱）
    termAttr.c_oflag = ONLCR | OPOST;
    // 本地模式：关闭回显！保留信号支持，关闭规范模式（关键）
    termAttr.c_lflag = ISIG;  // 仅保留信号（如Ctrl+C），关闭 ECHO/ICANON
    // 控制模式：8位数据、启用接收、本地终端（基础必选）
    termAttr.c_cflag = CS8 | CREAD | CLOCAL;

    // 波特率：匹配终端默认（无需显式设置9600，用系统默认更兼容）
    cfsetispeed(&termAttr, cfgetispeed(&termAttr));
    cfsetospeed(&termAttr, cfgetospeed(&termAttr));

    // 非阻塞读取配置（关键：VMIN=1 有数据就读，VTIME=0 不等待）
    termAttr.c_cc[VMIN]  = 1;  // 至少读1个字节才返回
    termAttr.c_cc[VTIME] = 0;  // 无超时（非阻塞）
}

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

QString formatPath(const QString &path)
{
    QString param = path;
    expandTilde(param);
    return param;
}

int calculateTargetFps(const QRect &diffRect, const QSize &screenSize, ClientInfo &info)
{
    if (diffRect.isEmpty())
    {
        return 0;  // 无差分，不发送
    }

    // 1. 计算差分区域占屏比（面积比）
    qreal screenArea   = screenSize.width() * screenSize.height();
    qreal diffArea     = diffRect.width() * diffRect.height();
    info.diffAreaRatio = diffArea / screenArea;  // 保存到客户端信息

    // 2. 根据占比动态调整帧率（可自定义阈值）
    int targetFps;
    if (info.diffAreaRatio >= 0.8)
    {  // 大区域（≥80%）
        targetFps = info.maxFps;
    }
    else if (info.diffAreaRatio >= 0.2)
    {  // 中等区域（20%~80%）
        // 线性插值：20%→20帧，80%→30帧
        targetFps = 20 + (info.diffAreaRatio - 0.2) / 0.6 * 10;
    }
    else
    {                             // 小区域（≤20%）
        targetFps = info.minFps;  // 最低15帧
    }

    // 3. 限制帧率范围（避免超出最小/最大值）
    targetFps = qBound(info.minFps, targetFps, info.maxFps);
    return targetFps;
}
