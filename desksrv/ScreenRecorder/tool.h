#ifndef TOOL_H
#define TOOL_H
#include <string>
class QString;
class QImage;
namespace cv{
    class Mat;
}
QImage cvMatToQImage(const cv::Mat &mat);

QString getGlobalStyle();

QString getSaveFilePath(const QString& fileName,const QString& suffix);

bool openFileDir(const QString& path);

// 水印绘制
void drawWatermark(cv::Mat& frame, const std::string& text);



#endif // TOOL_H

