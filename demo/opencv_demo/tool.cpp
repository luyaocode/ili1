#include "tool.h"
#include <QImage>
#include <QDebug>
#include <opencv2/opencv.hpp>

QImage cvMatToQImage(const cv::Mat &mat)
{
    // 处理 8 位 3 通道图像
    if (mat.type() == CV_8UC3)
    {
        // OpenCV 使用 BGR 格式，Qt 使用 RGB 格式，需要转换
        cv::Mat rgbMat;
        cv::cvtColor(mat, rgbMat, CV_BGR2RGB);
        return QImage((const unsigned char*)rgbMat.data,
                      rgbMat.cols, rgbMat.rows,
                      rgbMat.step,
                      QImage::Format_RGB888).copy();
    }
    // 处理 8 位单通道图像（灰度图）
    else if (mat.type() == CV_8UC1)
    {
        return QImage((const unsigned char*)mat.data,
                      mat.cols, mat.rows,
                      mat.step,
                      QImage::Format_Grayscale8).copy();
    }
    else
    {
        qDebug() << "不支持的图像格式";
        return QImage();
    }
}
