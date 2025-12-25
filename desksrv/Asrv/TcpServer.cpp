#include "TcpServer.h"
#include <QCoreApplication>
#include <QRegularExpression>
#include <QHostAddress>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QTextCodec>
#include <QScreen>
#include <QGuiApplication>
#include <QImage>
#include <QBuffer>
#include <QByteArray>
#include <QTextStream>
#include <QPixmap>
#include <QPainter>
#include <QTimer>
#include <QElapsedTimer>
#include <QMutex>
#include <QDateTime>
#include <QMessageBox>
#include <QApplication>
#include <QWidget>

#include "tool.h"
#include "def.h"
#include "Server.h"
#include "widget/NotifyPopup.h"

TcpServer::TcpServer(QObject *parent): QTcpServer(parent)
{
    QString   strRoot = formatPath(ROOT_DIR);
    QFileInfo fileInfo(strRoot);
    if (!(fileInfo.isDir() && fileInfo.exists()))
    {
        strRoot = "/";
    }
    m_rootDir = strRoot;
    qInfo() << "文件系统根目录映射：" << m_rootDir;
}

void TcpServer::startListen(const QString &serverIp, const quint16 serverPort)
{
    // 监听指定 IP 和端口
    if (!listen(QHostAddress(serverIp), serverPort))
    {
        qCritical() << "服务器启动失败：" << errorString();
        qApp->quit();
    }
    else
    {
        m_listenIp   = serverIp;
        m_listenPort = serverPort;
        qInfo() << "HTTP 服务器已启动";
        qInfo() << "访问地址：http://" << serverIp << ":" << serverPort;

        // 检查网页目录是否存在
        QDir wwwDir("./www");
        if (!wwwDir.exists())
        {
            qWarning() << "www目录不存在，可能导致网页无法加载";
        }
    }
}

void TcpServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket *clientSocket = new QTcpSocket(this);
    clientSocket->setSocketDescriptor(socketDescriptor);

    qDebug() << "新客户端连接：IP=" << clientSocket->peerAddress().toString() << "，Socket描述符=" << socketDescriptor;

    // ========== 关键1：配置Socket适配大文件 ==========
    clientSocket->setReadBufferSize(0);  // 禁用读缓冲区大小限制（0=无限制）
    // 增大接收缓冲区（建议设为最大请求体的1/10，避免内存占用过高）
    clientSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption, MAX_REQUEST_SIZE / 10);
    // 开启TCP保活，避免大文件上传时连接被断开
    clientSocket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    connect(clientSocket, &QTcpSocket::readyRead, this, [this, clientSocket]() {
        handleClientRequest(clientSocket);
    });
    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
    connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this,
            [clientSocket](QAbstractSocket::SocketError socketError) {
                qWarning() << "客户端连接错误：" << socketError << clientSocket->errorString();
            });
}

void TcpServer::onSocketDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket)
    {
        m_socketRequestCache.remove(socket);  // 移除对应socket的缓存
        socket->deleteLater();                // 释放socket资源
    }
}

void TcpServer::handleClientRequest(QTcpSocket *socket)
{
    if (!socket)
        return;

    // 获取客户端信息
    QHostAddress clientIp   = socket->peerAddress();
    quint16      clientPort = socket->peerPort();
    qDebug() << "客户端请求：IP=" << clientIp.toString() << "，Port=" << clientPort;

    // 读取请求数据
    QByteArray data = socket->readAll();
    qDebug().noquote() << "收到请求数据：\n" << data;

    // 1. 绑定断开连接的信号（防止内存泄漏）
    connect(socket, &QTcpSocket::disconnected, this, &TcpServer::onSocketDisconnected, Qt::UniqueConnection);

    // 2. 读取当前可用数据，追加到对应socket的缓存中
    m_socketRequestCache[socket] += data;
    QByteArray &requestData = m_socketRequestCache[socket];

    // ========== 新增：优先处理上传请求（POST /upload） ==========
    // 优先处理上传请求
    QRegularExpression postUploadRe("^POST /upload HTTP/[0-9.]+");
    if (postUploadRe.match(requestData).hasMatch())
    {
        // 调用检查函数，不再递归
        checkUploadDataComplete(socket, requestData);
        return;
    }

    // 解析GET请求路径
    QRegularExpression      re("^GET (/[^ ]*) HTTP/[0-9.]+");
    QRegularExpressionMatch match       = re.match(requestData);
    QString                 requestPath = match.captured(1);
    requestPath                         = decodeFilePath(requestPath);

    // 先处理静态资源请求
    // TODO：区分普通js文件和服务器js资源
    if (handleStaticResource(requestPath, socket) || handleScreenRequest(requestPath, socket) ||
        handleRTC(requestPath, socket) || handleNotify(requestPath, socket) || handleTest(requestPath, socket) ||
        handleBash(requestPath, socket) || handleControl(requestPath, socket) || handleXterm(requestPath, socket) ||
        handleScreenCtrl(requestPath, socket))
    {
        return;
    }

    bool isPreview = false;
    // 1. 查找X-File-Action: preview（忽略大小写和空格）
    if (requestData.contains("X-File-Action: preview") || requestData.contains("x-file-action: preview"))
    {
        // 2. 验证密钥（可选，增强安全性）
        if (requestData.contains("X-Preview-Key: your_secret_key_123"))
        {
            isPreview = true;
        }
    }

    // 默认路径：根目录
    if (requestPath.isEmpty() || requestPath == "/")
    {
        requestPath = "/";
    }

    // 拼接服务端路径
    QDir    rootDir(m_rootDir);
    QString serverFilePath = rootDir.filePath(requestPath.mid(1));
    serverFilePath.replace("\\", "/");
    qDebug() << "映射到服务端路径：" << serverFilePath;

    // 读取文件/生成目录列表
    QByteArray content = readFileOrDir(serverFilePath, m_rootDir, requestPath, isPreview);

    // 构建HTTP响应
    QByteArray response;
    if (content.contains("403 - 禁止访问"))
    {
        response += "HTTP/1.1 403 Forbidden\r\n";
    }
    else if (content.contains("404 - "))
    {
        response += "HTTP/1.1 404 Not Found\r\n";
    }
    else if (content.contains("500 - 文件不支持预览"))
    {
        response += "HTTP/1.1 500 No Support Preview\r\n";
    }
    else
    {
        response += "HTTP/1.1 200 OK\r\n";
    }

    // 修复：正确设置MIME类型
    QFileInfo fileInfo(serverFilePath);
    QString   mimeType;
    // 目录/错误页面强制HTML类型
    if (fileInfo.isDir() || content.contains("403 - 禁止访问") || content.contains("404 - "))
    {
        mimeType = "text/html; charset=UTF-8";
    }
    else
    {
        mimeType = getMimeType(serverFilePath);
    }
    // 预览特殊处理
    if (isPreview)
    {
        mimeType = "text/plain; charset=UTF-8";
    }
    qDebug() << "请求路径是否为目录：" << fileInfo.isDir();
    qDebug() << "最终MIME类型：" << mimeType;

    // 设置响应头
    response += "Content-Type: " + mimeType.toUtf8() + "\r\n";
    response += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "Cache-Control: no-cache\r\n";
    response += "\r\n";
    response += content;

    // 发送响应
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

bool TcpServer::handleStaticResource(const QString &requestPath, QTcpSocket *socket)
{
    // 1. 定义静态资源前缀（所有以这些前缀开头的请求都视为静态资源）
    const QStringList staticPrefixes  = {"/js/", "/css/", "/img/", "/fonts/"};
    bool              isStaticRequest = false;
    for (const QString &prefix : staticPrefixes)
    {
        if (requestPath.startsWith(prefix))
        {
            isStaticRequest = true;
            break;
        }
    }
    if (!isStaticRequest)
    {
        return false;  // 不是静态资源，返回false让后续逻辑处理
    }

    // 2. 拼接静态资源的真实路径（安装目录/www + 请求路径）
    QString realFilePath = QCoreApplication::applicationDirPath() + "/www" + requestPath;
    QFile   file(realFilePath);

    // 3. 打开文件并读取内容
    if (!file.open(QIODevice::ReadOnly))
    {
        // 文件不存在，返回404响应
        QByteArray response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: text/plain; charset=UTF-8\r\n";
        response += "Connection: close\r\n\r\n";
        response += "File not found: " + requestPath.toUtf8();
        socket->write(response);
        socket->flush();
        socket->disconnectFromHost();
        return true;
    }

    QByteArray fileContent = file.readAll();
    file.close();

    // 特殊处理：替换占位符
    QString fileName = QFileInfo(realFilePath).fileName();
    if (fileName == "bash.js" || fileName == "xtermpage.js")
    {
        QMap<QString, QString> replaceMap;
        QString                strWsAddr = "ws://" + getLocalIpv4() + ":" + QString::number(Server::getWsPort());
        replaceMap["WS_HOST"]            = strWsAddr;
        QString contentStr               = QString(fileContent);
        for (auto it = replaceMap.constBegin(); it != replaceMap.constEnd(); ++it)
        {
            contentStr.replace("{{" + it.key() + "}}", it.value());
        }

        fileContent = contentStr.toUtf8();
    }
    else if(fileName == "screen_ctrl.js")
    {
        QMap<QString, QString> replaceMap;
        QString                strWsAddr = "ws://" + getLocalIpv4() + ":" + QString::number(Server::getScreenServerPort());
        replaceMap["WS_HOST"]            = strWsAddr;
        QString contentStr               = QString(fileContent);
        for (auto it = replaceMap.constBegin(); it != replaceMap.constEnd(); ++it)
        {
            contentStr.replace("{{" + it.key() + "}}", it.value());
        }

        fileContent = contentStr.toUtf8();
    }

    // 4. 根据文件后缀获取MIME类型
    QString contentType = getMimeType(requestPath);

    // 5. 构建HTTP响应
    QByteArray response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: " + contentType.toUtf8() + "\r\n";
    response += "Content-Length: " + QByteArray::number(fileContent.size()) + "\r\n";
    response += "Connection: close\r\n";
    // 可选：添加缓存头，提升性能
    response += "Cache-Control: max-age=86400\r\n\r\n";
    response += fileContent;

    // 6. 发送响应
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();

    return true;
}

// 处理上传请求
void TcpServer::handleUploadRequest(QTcpSocket *socket, const QByteArray &requestData)
{
    // 3. 优先判断是否为上传请求（POST /upload）
    QRegularExpression postUploadRe("^POST /upload HTTP/[0-9.]+");
    if (postUploadRe.match(requestData).hasMatch())
    {
        // 3.1 解析Content-Length，判断数据是否完整
        QRegularExpression contentLengthRe(R"(Content-Length:\s*(\d+))", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch clMatch = contentLengthRe.match(requestData);
        if (clMatch.hasMatch())
        {
            qint64 expectedLength = clMatch.captured(1).toLongLong();
            int    headerEndPos   = requestData.indexOf("\r\n\r\n");

            // 计算已接收的请求体长度
            qint64 receivedBodyLength = requestData.size() - (headerEndPos + 4);

            // 数据完整：处理上传请求
            if (headerEndPos != -1 && receivedBodyLength >= expectedLength)
            {
                qDebug() << "上传请求数据完整，总长度：" << requestData.size();
                handleUploadRequest(socket, requestData);
                m_socketRequestCache.remove(socket);  // 清理缓存
                return;
            }
        }
        // 3.2  检查Multipart结束标记（--boundary--）
        QRegularExpression      boundaryRe(R"(Content-Type:\s*multipart/form-data;\s*boundary=([^\r\n;]+))",
                                           QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch boundaryMatch = boundaryRe.match(requestData);
        if (boundaryMatch.hasMatch())
        {
            QString    boundary  = boundaryMatch.captured(1);
            QByteArray endMarker = QString("--%1--\r\n").arg(boundary).toUtf8();
            if (requestData.contains(endMarker))
            {
                handleUploadRequest(socket, requestData);
                m_socketRequestCache.remove(socket);
                return;
            }
        }
        // 数据未完整，等待下一次readyRead
        qDebug() << "上传请求数据未完整，当前接收长度：" << requestData.size();
        return;
    }
}

bool TcpServer::checkUploadDataComplete(QTcpSocket *socket, const QByteArray &requestData)
{
    // 1. 解析Content-Length，判断数据是否完整
    QRegularExpression      contentLengthRe(R"(Content-Length:\s*(\d+))", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch clMatch = contentLengthRe.match(requestData);
    if (clMatch.hasMatch())
    {
        qint64 expectedLength = clMatch.captured(1).toLongLong();
        int    headerEndPos   = requestData.indexOf("\r\n\r\n");

        if (headerEndPos == -1)
            return false;

        // 计算已接收的请求体长度
        qint64 receivedBodyLength = requestData.size() - (headerEndPos + 4);

        // 数据完整：调用实际处理函数
        if (receivedBodyLength >= expectedLength)
        {
            qDebug() << "上传请求数据完整，总长度：" << requestData.size();
            doHandleUploadRequest(socket, requestData);
            m_socketRequestCache.remove(socket);  // 清理缓存
            return true;
        }
    }

    // 2. 兜底：检查Multipart结束标记（--boundary--）
    QRegularExpression      boundaryRe(R"(Content-Type:\s*multipart/form-data;\s*boundary=([^\r\n;]+))",
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch boundaryMatch = boundaryRe.match(requestData);
    if (boundaryMatch.hasMatch())
    {
        QString boundary = boundaryMatch.captured(1);
        QByteArray endMarker = QString("--%1--").arg(boundary).toUtf8();  // 注意：结束标记可能没有最后的\r\n
        if (requestData.contains(endMarker))
        {
            doHandleUploadRequest(socket, requestData);
            m_socketRequestCache.remove(socket);
            return true;
        }
    }

    // 数据未完整
    qDebug() << "上传请求数据未完整，当前接收长度：" << requestData.size();
    return false;
}

void TcpServer::doHandleUploadRequest(QTcpSocket *socket, const QByteArray &requestData)
{
    // ========== 1. 提取请求体 ==========
    int headerEndPos = requestData.indexOf("\r\n\r\n");
    if (headerEndPos == -1)
    {
        sendJsonResponse(socket, 400, "HTTP请求格式错误：未找到请求头与体的分隔符");
        return;
    }

    QByteArray bodyData = requestData.mid(headerEndPos + 4);
    if (bodyData.isEmpty())
    {
        sendJsonResponse(socket, 400, "请求体为空");
        return;
    }

    // ========== 2. 解析Boundary ==========
    QRegularExpression      boundaryRe(R"(Content-Type:\s*multipart/form-data;\s*boundary=([^\r\n;]+))",
                                       QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch boundaryMatch = boundaryRe.match(requestData);
    if (!boundaryMatch.hasMatch())
    {
        sendJsonResponse(socket, 400, "缺少multipart边界符");
        return;
    }

    QString boundary = boundaryMatch.captured(1);
    qDebug() << "解析到Boundary：" << boundary;
    qDebug() << "bodyData长度：" << bodyData.size();

    // ========== 3. 解析表单数据 ==========
    QMap<QString, QString>            formFields;
    QList<QPair<QString, QByteArray>> fileDataList;

    QByteArray        boundaryBytes = QString("--" + boundary).toUtf8();
    QList<QByteArray> parts;
    int               pos         = 0;
    int               boundaryLen = boundaryBytes.length();
    while (pos < bodyData.length())
    {
        int nextPos = bodyData.indexOf(boundaryBytes, pos);
        if (nextPos == -1)
            break;
        QByteArray part = bodyData.mid(pos, nextPos - pos);
        parts.append(part);
        pos = nextPos + boundaryLen;
    }

    for (QByteArray &part : parts)
    {
        part = part.trimmed();
        if (part.isEmpty() || part == "--")
            continue;

        int headerEnd = part.indexOf("\r\n\r\n");
        if (headerEnd == -1)
            continue;

        QByteArray header  = part.left(headerEnd);
        QByteArray content = part.mid(headerEnd + 4);

        // 移除内容末尾的结束标记
        int contentEnd = content.lastIndexOf("\r\n--");
        if (contentEnd != -1)
        {
            content = content.left(contentEnd);
        }

        // 解析字段名和文件名
        QRegularExpression nameRe("name\\s*=\\s*\"([^\"]+)\"", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression filenameRe("filename\\s*=\\s*\"([^\"]+)\"", QRegularExpression::CaseInsensitiveOption);

        QRegularExpressionMatch nameMatch     = nameRe.match(header);
        QRegularExpressionMatch filenameMatch = filenameRe.match(header);

        if (!nameMatch.hasMatch())
            continue;

        QString fieldName = nameMatch.captured(1);
        QString fileName  = filenameMatch.hasMatch() ? filenameMatch.captured(1) : QString();

        if (!fileName.isEmpty())
        {
            fileDataList.append(qMakePair(fileName, content));
        }
        else
        {
            formFields.insert(fieldName, QString::fromUtf8(content));
        }
    }

    if (formFields.isEmpty() && fileDataList.isEmpty())
    {
        sendJsonResponse(socket, 400, "表单解析失败：无有效字段或文件");
        return;
    }

    // ========== 4. 处理上传路径 ==========
    QString uploadDir = m_rootDir + "/upload";

    // 创建上传目录
    QDir dir;
    if (!dir.exists(uploadDir) && !dir.mkpath(uploadDir))
    {
        sendJsonResponse(socket, 500, "创建上传目录失败");
        return;
    }

    // ========== 5. 写入文件 ==========
    bool    uploadSuccess = true;
    QString errorMsg;

    for (const auto &filePair : fileDataList)
    {
        QString    fileName    = filePair.first;
        QByteArray fileContent = filePair.second;

        // 处理中文文件名
        fileName = QUrl::fromPercentEncoding(fileName.toUtf8());
        if (fileName.isEmpty())
        {
            uploadSuccess = false;
            errorMsg      = "文件名不能为空";
            break;
        }

        QString filePath = uploadDir + "/" + fileName;
        QFile   file(filePath);

        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            uploadSuccess = false;
            errorMsg      = "文件打开失败：" + file.errorString();
            break;
        }

        qint64 written = file.write(fileContent);
        file.close();

        if (written != fileContent.size())
        {
            uploadSuccess = false;
            errorMsg      = "文件写入不完整";
            QFile::remove(filePath);
            break;
        }

        qDebug() << "文件上传成功：" << filePath;
    }

    // ========== 6. 发送响应 ==========
    if (uploadSuccess)
    {
        sendJsonResponse(socket, 200, "文件上传成功");
    }
    else
    {
        sendJsonResponse(socket, 500, errorMsg);
    }
}

bool TcpServer::handleScreenRequest(const QString &requestPath, QTcpSocket *socket)
{
    // 1. 校验请求路径是否匹配截屏指令
    if (!requestPath.startsWith(REQ_SCREEN))
    {
        return false;  // 非截屏请求，返回false交给后续逻辑处理
    }

    // 2. 服务端截屏核心逻辑（兼容多屏幕场景）
    QImage screenshotImg;
    // 获取系统所有屏幕
    QList<QScreen *> screens = QGuiApplication::screens();
    if (screens.isEmpty())
    {
        // 无可用屏幕，返回500错误
        sendJsonResponse(socket, 500, "No available screen to capture");
        return true;
    }

    // 方式1：截取主屏幕（单屏幕场景）
    //    QScreen *primaryScreen = QGuiApplication::primaryScreen();
    //    screenshotImg          = primaryScreen->grabWindow(0).toImage();  // 0表示截取整个屏幕

    // 方式2：截取所有屏幕拼接（多屏幕场景，可选）
    QPixmap totalScreenshot;
    int     totalWidth = 0, totalHeight = 0;
    for (QScreen *screen : screens)
    {
        totalWidth += screen->geometry().width();
        totalHeight = qMax(totalHeight, screen->geometry().height());
    }
    totalScreenshot = QPixmap(totalWidth, totalHeight);
    QPainter painter(&totalScreenshot);
    int      xOffset = 0;
    for (QScreen *screen : screens)
    {
        QPixmap screenPix = screen->grabWindow(0);
        painter.drawPixmap(xOffset, 0, screenPix);
        xOffset += screen->geometry().width();
    }
    screenshotImg = totalScreenshot.toImage();

    // 3. 校验截屏是否成功
    if (screenshotImg.isNull())
    {
        sendJsonResponse(socket, 500, "Failed to capture screen image");
        return true;
    }

    // 4. 将截屏图片转为Base64编码（嵌入HTML用）
    QByteArray imageData;
    QBuffer    buffer(&imageData);
    buffer.open(QIODevice::WriteOnly);
    // 保存为JPG格式（体积更小），可改为PNG（质量更高）
    screenshotImg.save(&buffer, "JPG", 85);  // 85为JPG质量（0-100）
    QString base64Img = QString(imageData.toBase64());

    // 5. 构建包含Base64图片的HTML响应
    QMap<QString, QString> replaceMap;
    replaceMap["BASE64_IMG"] = base64Img;
    QString htmlContent      = QString::fromUtf8(readTemplate("screen.html", replaceMap));

    // 6. 构建HTTP响应
    QByteArray  response;
    QTextStream respStream(&response);
    respStream << "HTTP/1.1 200 OK\r\n";
    respStream << "Content-Type: text/html; charset=UTF-8\r\n";
    respStream << "Content-Length: " << htmlContent.size() << "\r\n";
    respStream << "Connection: close\r\n\r\n";
    respStream << htmlContent;

    // 7. 发送响应并关闭连接
    if (socket->isWritable())
    {
        socket->write(response);
        socket->flush();  // 强制刷出缓冲区
    }
    socket->disconnectFromHost();

    return true;
}

bool TcpServer::handleRTC(const QString &requestPath, QTcpSocket *socket)
{
    // 1. 校验请求路径是否匹配截屏指令
    if (!requestPath.startsWith(REQ_RTC))
    {
        return false;
    }

    // 2. 防止重复推流（同一socket只创建一个定时器）
    QMutexLocker locker(&m_timerMutex);
    if (m_screenStreamTimers.contains(socket))
    {
        return true;  // 已有推流，直接返回
    }

    // 3. 发送初始化响应头（开启multipart长连接）
    QByteArray initResponse = "HTTP/1.1 200 OK\r\n";
    initResponse += "Content-Type: multipart/x-mixed-replace; boundary=--screenBoundary\r\n";
    initResponse += "Connection: keep-alive\r\n";   // 保持长连接
    initResponse += "Cache-Control: no-cache\r\n";  // 禁用缓存
    initResponse += "Pragma: no-cache\r\n\r\n";
    if (socket->isWritable())
    {
        socket->write(initResponse);
        socket->flush();
    }

    // 4. 创建帧率定时器（25帧/秒 = 40ms/帧）
    QTimer *frameTimer = new QTimer(this);
    frameTimer->setInterval(40);  // 40ms触发一次，≈25帧/秒
    m_screenStreamTimers[socket] = frameTimer;

    // 5. 关联定时器槽函数（循环截屏+推送）
    connect(frameTimer, &QTimer::timeout, this, [=]() {
        // 检查socket是否有效
        if (!socket || socket->state() != QTcpSocket::ConnectedState)
        {
            stopRTC(socket);
            return;
        }

        // 6. 截屏逻辑（多屏幕拼接）
        QImage           screenshotImg;
        QList<QScreen *> screens = QGuiApplication::screens();
        if (screens.isEmpty())
        {
            sendJsonResponse(socket, 500, "No available screen to capture");
            stopRTC(socket);
            return;
        }

        QPixmap totalScreenshot;
        int     totalWidth = 0, totalHeight = 0;
        for (QScreen *screen : screens)
        {
            totalWidth += screen->geometry().width();
            totalHeight = qMax(totalHeight, screen->geometry().height());
        }
        totalScreenshot = QPixmap(totalWidth, totalHeight);
        QPainter painter(&totalScreenshot);
        int      xOffset = 0;
        for (QScreen *screen : screens)
        {
            QPixmap screenPix = screen->grabWindow(0);
            painter.drawPixmap(xOffset, 0, screenPix);
            xOffset += screen->geometry().width();
        }
        screenshotImg = totalScreenshot.toImage();

        if (screenshotImg.isNull())
        {
            sendJsonResponse(socket, 500, "Failed to capture screen image");
            stopRTC(socket);
            return;
        }

        // 7. 将图片转为JPG（压缩体积，提升推流效率）
        QByteArray imageData;
        QBuffer    buffer(&imageData);
        buffer.open(QIODevice::WriteOnly);
        screenshotImg.save(&buffer, "JPG", 80);  // 80质量，平衡体积和清晰度

        // 8. 构建multipart分片（每帧图片作为一个分片）
        QByteArray frameData;
        frameData += "--screenBoundary\r\n";  // 分隔符
        frameData += "Content-Type: image/jpeg\r\n";
        frameData += "Content-Length: " + QByteArray::number(imageData.size()) + "\r\n\r\n";
        frameData += imageData;
        frameData += "\r\n";

        // 9. 推送单帧数据
        if (socket->isWritable())
        {
            socket->write(frameData);
            socket->flush();  // 强制刷出，避免数据堆积
        }
    });

    // 10. 监听socket断开事件（客户端关闭则停止推流）
    connect(socket, &QTcpSocket::disconnected, this, [=]() {
        stopRTC(socket);
    });
    connect(socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error), this, [=]() {
        stopRTC(socket);
    });

    // 11. 启动定时器，开始推流
    frameTimer->start();

    return true;
}

void TcpServer::stopRTC(QTcpSocket *socket)
{
    QMetaObject::invokeMethod(this, "slotStopScreenStream", Qt::QueuedConnection, Q_ARG(QTcpSocket *, socket));
}

void TcpServer::slotStopScreenStream(QTcpSocket *socket)
{
    QMutexLocker locker(&m_timerMutex);
    if (m_screenStreamTimers.contains(socket))
    {
        QTimer *timer = m_screenStreamTimers.take(socket);
        timer->stop();
        timer->deleteLater();
        m_screenStreamTimers.remove(socket);
        if (socket)
        {
            socket->disconnectFromHost();
            socket->deleteLater();  // 按需释放，根据你的连接管理逻辑调整
        }
    }
}

bool TcpServer::handleNotify(const QString &requestPath, QTcpSocket *socket)
{
    // 1. 校验请求路径是否匹配截屏指令
    if (!requestPath.startsWith(REQ_NOTIFY))
    {
        return false;
    }
    QString notifyMessage = requestPath.mid(REQ_NOTIFY.length());
    // 安全处理：解码URL编码（避免空格/特殊字符被转义，如%20→空格）
    notifyMessage = QUrl::fromPercentEncoding(notifyMessage.toUtf8());
    // 空消息校验
    if (notifyMessage.isEmpty())
    {
        sendJsonResponse(socket, 400, "Notify message is empty");
        return true;
    }

    // 3. 跨线程安全触发弹窗（关键：Qt GUI操作必须在主线程）
    // 若HttpServer在主线程，直接调用；否则用invokeMethod切换到主线程
    showNotifyPopup(notifyMessage);

    // 4. 发送成功响应给客户端
    sendJsonResponse(socket, 200, QString("Notify shown: %1").arg(notifyMessage));
    return true;
}

void TcpServer::showNotifyPopup(const QString &message)
{
    // 1. 禁止关闭最后一个窗口时退出程序（全局一次即可，可移到main函数）
    QApplication::setQuitOnLastWindowClosed(false);

    // 2. 创建自定义弹窗（标题固定为“系统通知”，也可作为参数传入）
    NotifyPopup *popup = new NotifyPopup("系统通知", message);

    // 3. 计算屏幕右下角贴边位置（无额外边距，完全贴边）
    QScreen *screen         = QApplication::primaryScreen();
    QRect    screenGeometry = screen->availableGeometry();  // 排除任务栏的可用区域
    QSize    popupSize      = popup->sizeHint();            // 获取弹窗推荐尺寸

    // 右下角坐标：屏幕宽度 - 弹窗宽度，屏幕高度 - 弹窗高度（完全贴边）
    int x = screenGeometry.width() - popupSize.width();
    int y = screenGeometry.height() - popupSize.height();

    // 防止坐标为负数（比如弹窗比屏幕大，仅做容错）
    x = qMax(0, x);
    y = qMax(0, y);

    popup->move(x, y);

    // 4. 显示弹窗
    popup->show();
}

bool TcpServer::handleTest(const QString &requestPath, QTcpSocket *socket)
{
    // 1. 校验请求路径是否匹配截屏指令
    if (!requestPath.startsWith(REQ_TEST))
    {
        return false;
    }
    auto content = readTemplate("test_ws.html");
    // 构建HTTP响应
    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=UTF-8\r\n";
    response += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "Cache-Control: no-cache\r\n";
    response += "\r\n";
    response += content;

    // 发送响应
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();

    return true;
}

bool TcpServer::handleBash(const QString &requestPath, QTcpSocket *socket)
{
    // 1. 校验请求路径是否匹配截屏指令
    if (!requestPath.startsWith(REQ_BASH))
    {
        return false;
    }
    QMap<QString, QString> replaceMap;
    QString                strWsAddr = "ws://" + getLocalIpv4() + ":" + QString::number(Server::getWsPort());
    replaceMap["WS_HOST"]            = strWsAddr;
    auto content                     = readTemplate("bash.html", replaceMap);
    // 构建HTTP响应
    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=UTF-8\r\n";
    response += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "Cache-Control: no-cache\r\n";
    response += "\r\n";
    response += content;

    // 发送响应
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();

    return true;
}

bool TcpServer::handleControl(const QString &requestPath, QTcpSocket *socket)
{
    // 1. 校验请求路径是否匹配截屏指令
    if (!requestPath.startsWith(REQ_CTRL))
    {
        return false;
    }

    // 2. 返回分屏HTML页面（核心修改）
    auto htmlContent = readTemplate("control.html");

    // 发送HTML响应头
    QByteArray response = "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=utf-8\r\n";
    response += "Content-Length: " + QByteArray::number(htmlContent.size()) + "\r\n";
    response += "Connection: close\r\n\r\n";
    response += htmlContent;

    if (socket->isWritable())
    {
        socket->write(response);
        socket->flush();
        // 发送完HTML后关闭连接（前端会通过新请求连接WebSocket和屏幕流）
        QTimer::singleShot(1000, socket, &QTcpSocket::close);
    }

    return true;
}

bool TcpServer::handleXterm(const QString &requestPath, QTcpSocket *socket)
{
    // 1. 校验请求路径是否匹配截屏指令
    if (!requestPath.startsWith(REQ_XTERM))
    {
        return false;
    }
    QMap<QString, QString> replaceMap;
    //    QString                strWsAddr = "ws://" + getLocalIpv4() + ":" + QString::number(Server::getWsPort());
    //    replaceMap["WS_HOST"]            = strWsAddr;
    auto content = readTemplate("xterm.html", replaceMap);
    // 构建HTTP响应
    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=UTF-8\r\n";
    response += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "Cache-Control: no-cache\r\n";
    response += "\r\n";
    response += content;

    // 发送响应
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();

    return true;
}

bool TcpServer::handleScreenCtrl(const QString &requestPath, QTcpSocket *socket)
{
    // 1. 校验请求路径是否匹配截屏指令
    if (!requestPath.startsWith(REQ_SCREEN_CTRL))
    {
        return false;
    }
    QMap<QString, QString> replaceMap;
    //    QString                strWsAddr = "ws://" + getLocalIpv4() + ":" + QString::number(Server::getWsPort());
    //    replaceMap["WS_HOST"]            = strWsAddr;
    auto content = readTemplate("screen_ctrl.html", replaceMap);
    // 构建HTTP响应
    QByteArray response;
    response += "HTTP/1.1 200 OK\r\n";
    response += "Content-Type: text/html; charset=UTF-8\r\n";
    response += "Content-Length: " + QByteArray::number(content.size()) + "\r\n";
    response += "Connection: close\r\n";
    response += "Cache-Control: no-cache\r\n";
    response += "\r\n";
    response += content;

    // 发送响应
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();

    return true;
}
