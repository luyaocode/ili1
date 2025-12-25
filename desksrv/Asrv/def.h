#ifndef DEF_H
#define DEF_H
#include <QString>
// 全局常量：MIME 类型映射（静态常量，仅初始化一次）
static const QMap<QString, QString> MIME_MAP = []() -> QMap<QString, QString> {
    QMap<QString, QString> map;
    // 基础网页类型
    map.insert("html", "text/html; charset=UTF-8");
    map.insert("js", "application/javascript; charset=UTF-8");
    map.insert("css", "text/css; charset=UTF-8");
    // 图片类型
    map.insert("png", "image/png");
    map.insert("jpg", "image/jpeg");
    map.insert("jpeg", "image/jpeg");  // 补充 jpeg 后缀兼容
    map.insert("gif", "image/gif");
    map.insert("ico", "image/x-icon");
    // 文本类型
    map.insert("txt", "text/plain; charset=UTF-8");
    map.insert("json", "application/json; charset=UTF-8");
    map.insert("xml", "text/xml; charset=UTF-8");
    // 文档/压缩类型
    map.insert("pdf", "application/pdf");
    map.insert("zip", "application/zip");
    map.insert("rar", "application/x-rar-compressed");  // 补充 rar 类型
    // 二进制类型
    // 注：默认值不在常量映射中，由获取函数兜底返回
    return map;
}();

#define QS(x) QStringLiteral(x)
#define SAFE_DELETE(p) if(p) {delete p;p=nullptr;}

#define ROOT_DIR "~"
#define IP_ADDR "0.0.0.0"
#define IP_PORT 8080

// 定义预览文件的大小阈值（单位：字节，示例为10MB，可根据需求调整）
// 1MB = 1024 * 1024 字节，10MB = 10 * 1024 * 1024
const int MAX_PREVIEW_SIZE = 10 * 1024 * 1024;
const int MAX_REQUEST_SIZE = 200 * 1024 * 1024;

#define REQ_TEST QS("/$$test")
#define REQ_SCREEN QS("/$$screen")
#define REQ_RTC QS("/$$rtc")
#define REQ_NOTIFY QS("/$$notify/")
#define REQ_BASH QS("/$$bash")
#define REQ_CTRL QS("/$$ctrl")
#define REQ_XTERM QS("/$$xterm")
#define REQ_SCREEN_CTRL QS("/$$scc")





#endif  // DEF_H
