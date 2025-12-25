#include "tool.h"
#include <QImage>
#include <QString>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QMessageBox>
#include <opencv2/opencv.hpp>

QImage cvMatToQImage(const cv::Mat &mat)
{
    // 处理 8 位 3 通道图像
    if (mat.type() == CV_8UC3)
    {
        // OpenCV 使用 BGR 格式，Qt 使用 RGB 格式，需要转换
        cv::Mat rgbMat;
        cv::cvtColor(mat, rgbMat, CV_BGR2RGB);
        return QImage((const unsigned char *)rgbMat.data, rgbMat.cols, rgbMat.rows, rgbMat.step, QImage::Format_RGB888)
            .copy();
    }
    // 处理 8 位单通道图像（灰度图）
    else if (mat.type() == CV_8UC1)
    {
        return QImage((const unsigned char *)mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    }
    else
    {
        qDebug() << "不支持的图像格式";
        return QImage();
    }
}

QString getGlobalStyle()
{
    return "QMainWindow {"
           "    background-color: black;"
           "}"
           "QPushButton {"
           "    background-color: #2a7fff;"  // 亮蓝色背景，与黑色形成对比
           "    color: white;"               // 白色文字
           "    font-size: 12px;"            // 标准字号
           "    font-weight: bold;"          // 加粗文字
           "    padding: 4px 10px;"          // 内边距
           "    border-radius: 6px;"         // 圆角设计
           "    min-width: 60px;"            // 最小宽度
           "    margin: 1px;"                // 外边距
           "}"
           "QPushButton:hover {"
           "    background-color: #4d94ff;"  // 悬停时变亮
           "    border-color: white;"        // 边框变白
           "}"
           "QPushButton:pressed {"
           "    background-color: #1a66cc;"  // 按下时变暗
           "}"
           "QPushButton:disabled {"
           "    background-color: #444444;"  // 禁用状态
           "    color: #888888;"
           "    border-color: #666666;"
           "}";
}

QString getSaveFilePath(const QString &fileName, const QString &suffix)
{
    QFileInfo fileInfo(fileName);
    if (fileInfo.isDir())
    {
        qWarning() << "文件名是一个目录: " << fileName;
        return {};
    }
    // 获取路径
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    if (dataDir.isEmpty())
    {
        dataDir = QDir::currentPath();  // 降级处理
    }

    // 生成日期目录名（格式：yyyyMMdd）
    QString dateDirName = QDateTime::currentDateTime().toString("yyyyMMdd");
    // 拼接日期目录的完整路径（桌面路径 + 日期目录名）
    QString dateDirPath = QDir(dataDir).filePath(dateDirName);

    // 创建日期目录（如果不存在则递归创建）
    QDir dateDir(dateDirPath);
    if (!dateDir.exists())
    {
        if (!dateDir.mkpath("."))
        {  // "." 表示当前目录（即dateDirPath）
            qWarning() << "无法创建目录: " << dateDirPath;
            return {};
        }
    }
    QString targetName = fileName;
    if (targetName.isEmpty())
    {
        // 生成带时间戳的文件名（格式：screen_recording_yyyyMMdd_HHmmss.mp4）
        QString timeStamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        targetName        = QString("screen_%1").arg(timeStamp);
    }
    // 拼接最终的文件路径（日期目录 + 文件名）
    QString targetPath = QDir(dateDirPath).filePath(targetName);
    // 加后缀
    if (!targetPath.endsWith(suffix))
    {
        targetPath += suffix;
    }
    return targetPath;
}

bool openFileDir(const QString &path)
{
    // 使用系统默认的文件管理器打开桌面目录
    QFileInfo fileInfo(path);
    QString dir = fileInfo.absoluteDir().absolutePath();
    QUrl url = QUrl::fromLocalFile(dir);
    return QDesktopServices::openUrl(url);
}

// 圆角矩形绘制（分离填充和线宽参数）
void drawRoundedRect(
    cv::Mat& img,
    cv::Rect rect,
    cv::Scalar color,
    int fill = -1,  // 填充模式：-1=填充，0=不填充
    int thickness = 2,  // 边框线宽（仅当fill=0时有效）
    int lineType = cv::LINE_AA,
    int radius = 5
)
{
    // 确保线宽有效（满足OpenCV断言）
    if (thickness <= 0 && fill == 0) {
        thickness = 1;  // 默认为1px线宽
    }

    // 绘制主体矩形（填充或边框）
    if (fill == -1) {
        // 填充模式：先绘制填充的圆角矩形
        cv::Mat roi = img(rect);
        cv::Mat mask(rect.size(), CV_8UC1, cv::Scalar(0));
        drawRoundedRect(mask, cv::Rect(0, 0, rect.width, rect.height), cv::Scalar(255), 0, mask.cols, lineType, radius);
        color[3] = color[3] / 255.0;  // 处理alpha通道（如果有）
        cv::addWeighted(roi, 1.0, cv::Mat(roi.size(), roi.type(), color), color[3], 0, roi, roi.depth());
    }

    // 绘制边框线条（无论是否填充，都可绘制边框）
    // 上下边
    cv::line(img,
             cv::Point(rect.x + radius, rect.y),
             cv::Point(rect.x + rect.width - radius, rect.y),
             color, thickness, lineType);
    cv::line(img,
             cv::Point(rect.x + radius, rect.y + rect.height),
             cv::Point(rect.x + rect.width - radius, rect.y + rect.height),
             color, thickness, lineType);
    // 左右边
    cv::line(img,
             cv::Point(rect.x, rect.y + radius),
             cv::Point(rect.x, rect.y + rect.height - radius),
             color, thickness, lineType);
    cv::line(img,
             cv::Point(rect.x + rect.width, rect.y + radius),
             cv::Point(rect.x + rect.width, rect.y + rect.height - radius),
             color, thickness, lineType);
    // 四个角的圆弧
    cv::ellipse(img,
                cv::Point(rect.x + radius, rect.y + radius),
                cv::Size(radius, radius), 180, 0, 90,
                color, thickness, lineType);
    cv::ellipse(img,
                cv::Point(rect.x + rect.width - radius, rect.y + radius),
                cv::Size(radius, radius), 270, 0, 90,
                color, thickness, lineType);
    cv::ellipse(img,
                cv::Point(rect.x + radius, rect.y + rect.height - radius),
                cv::Size(radius, radius), 90, 0, 90,
                color, thickness, lineType);
    cv::ellipse(img,
                cv::Point(rect.x + rect.width - radius, rect.y + rect.height - radius),
                cv::Size(radius, radius), 0, 0, 90,
                color, thickness, lineType);
}

void drawWatermark(cv::Mat &frame, const std::string &text)
{
    if (frame.empty()) return;

    // 水印配置参数
    const int margin = 15;               // 边距
    const int cornerRadius = 8;          // 圆角半径
    const cv::Scalar bgColor(30, 30, 30, 180);  // 深色半透明背景（BGR+alpha）
    const cv::Scalar textColor(255, 215, 0);     // 金色文本（BGR）
    const cv::Scalar accentColor(0, 180, 255);   // 亮蓝色装饰（BGR）
    const int fontFace = cv::FONT_HERSHEY_COMPLEX; // 更精致的字体
    const double fontScale = 0.7;
    const int textThickness = 2;
    const int lineType = cv::LINE_AA;    // 抗锯齿

    // 1. 计算文本尺寸
    int baseline = 0;
    cv::Size textSize = cv::getTextSize(text, fontFace, fontScale, textThickness, &baseline);

    // 2. 计算水印区域（带装饰空间的扩展矩形）
    cv::Rect watermarkRect(
        frame.cols - textSize.width - margin * 2,  // x起点（右对齐）
        frame.rows - textSize.height - baseline - margin * 2,  // y起点（下对齐）
        textSize.width + margin * 2,              // 宽度（文本+左右边距）
        textSize.height + baseline + margin * 2   // 高度（文本+上下边距）
    );

    // 3. 绘制带圆角的半透明背景（增强质感）
    drawRoundedRect(
        frame,
        watermarkRect,
        bgColor,
        -1,  // 填充背景
        2,   // 边框线宽（正数，满足断言）
        lineType,
        cornerRadius
    );

    // 4. 绘制装饰性线条（左上角和右下角斜角装饰）
    const int decorLen = 12;  // 装饰线长度
    // 左上角斜线条
    cv::line(frame,
             watermarkRect.tl(),
             cv::Point(watermarkRect.x + decorLen, watermarkRect.y + decorLen),
             accentColor, 2, lineType);
    // 右下角斜线条
    cv::line(frame,
             watermarkRect.br(),
             cv::Point(watermarkRect.x + watermarkRect.width - decorLen, watermarkRect.y + watermarkRect.height - decorLen),
             accentColor, 2, lineType);

    // 5. 绘制文本阴影（增强立体感）
    cv::Point textOrg(
        watermarkRect.x + margin,
        watermarkRect.y + margin + textSize.height
    );
    cv::putText(frame, text, textOrg + cv::Point(1, 1),  // 向右下偏移1px作为阴影
                fontFace, fontScale, cv::Scalar(0, 0, 0, 150), textThickness, lineType);

    // 6. 绘制主文本（金色）
    cv::putText(frame, text, textOrg,
                fontFace, fontScale, textColor, textThickness, lineType);
}
