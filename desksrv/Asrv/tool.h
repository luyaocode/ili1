#ifndef TOOL_H
#define TOOL_H
#include <QByteArray>
#include <QString>
#include <QMap>

#include "def.h"

class QTcpSocket;
class QProcess;
class QWebSocket;

QString getMimeType(const QString &filePath);

// 读取模板文件并替换变量（key: 占位符名，value: 替换值）
QByteArray readTemplate(const QString &templateName, const QMap<QString, QString> &variables = {});

// 生成目录列表的HTML内容（基于模板）
QByteArray generateDirListHtml(const QString &dirPath, const QString &rootDir, const QString &requestPath);

// 路径安全校验：判断输入路径是否是映射根目录的子路径（防止路径遍历）
bool isValidPath(const QString &filePath, const QString &rootDir);

QByteArray
    readFileOrDir(const QString &serverFilePath, const QString &rootDir, const QString &requestPath, bool isPreview);

// 辅助函数：格式化文件大小（可选）
QString formatFileSize(qint64 bytes);

// 文件上传
// 解析分块传输（Chunked）数据
QByteArray parseChunkedData(const QByteArray &data);

// 发送通用响应
void sendJsonResponse(QTcpSocket *socket, int statusCode, const QString &message);

/**
 * @brief 解码 URL 中的中文路径
 * @param encodedPath 编码后的路径（如 share/%E6%96%B0%E5%BB%BA%E6%96%87%E6%9C%AC%E6%96%87%E6%A1%A3.txt）
 * @return 解码后的中文路径
 */
QString decodeFilePath(const QString &encodedPath);


QString getLocalIpv4();

// ANSI 转义序列转 HTML 样式
QString ansiToHtml(const QByteArray &input);

// 过滤 ANSI 转义序列（移除颜色、光标控制等）
QByteArray filterAnsiEscapeCodes(const QByteArray &input);

void setTermAttr(struct termios& termAttr);

void setTermAttrAsync(struct termios& termAttr);

// 路径转换
QString formatPath(const QString& path);

#endif  // TOOL_H
