#ifndef IMAGETOOL_H
#define IMAGETOOL_H
#include <opencv2/opencv.hpp>
#include <QImage>

// 辅助函数：Qt QImage 转 OpenCV Mat（保留）
cv::Mat qImageToMat(const QImage &image);

// 辅助函数：OpenCV Mat 转 Qt QImage（保留）
QImage matToQImage(const cv::Mat &mat);

// 图像增强
QImage enhanceImageQuality(const QImage &srcImage);




#endif // IMAGETOOL_H
