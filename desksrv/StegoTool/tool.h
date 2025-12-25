#ifndef TOOL_H
#define TOOL_H
#include <QString>
#include <QImage>
#include <QStringList>

#define U8(x) QString::fromUtf8(x)

bool embedText(const QString &strSource, const QString &strTarget, const QString &strText, int bitCount = 1);
bool extractText(const QString &strSource, QString &text, int bitCount = 0);

/**
 * @brief 执行 g++ 编译（独立包装函数，用户体验=直接运行 g++）
 * @param gppArgs 传递给 g++ 的完整参数列表（对应 ./StegoTool -c 后的所有内容）
 * @return int g++ 的退出码（0=成功，非0=失败）
 * @note 跨平台兼容（Windows/Linux/Mac），输出行为与 g++ 原生一致
 */
int executeGcc(const QStringList &gppArgs);

QImage createImage(int width, int height, int grayValue = 128);

#include <QApplication>
#include <QImage>
#include <QPainter>
#include <QLinearGradient>
#include <QBrush>
#include <QPen>
#include <QPainterPath>
#include <QDebug>
#include <QFileInfo>

/**
 * @brief 通用恐龙图像生成函数
 * @param width 图像宽度（像素，需≥100）
 * @param height 图像高度（像素，需≥100）
 * @param bgColor 画布背景色（默认白色）
 * @param mainColor 恐龙主色（默认绿色，会自动生成渐变）
 * @return bool 生成成功返回true，失败返回false
 */
QImage generateDinosaurImage(int           width,
                             int           height,
                             const QColor &bgColor   = Qt::white,
                             const QColor &mainColor = QColor(100, 180, 100));

#endif  // TOOL_H
