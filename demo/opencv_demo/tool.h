#ifndef TOOL_H
#define TOOL_H

class QImage;
namespace cv{
    class Mat;
}
QImage cvMatToQImage(const cv::Mat &mat);

#endif // TOOL_H
