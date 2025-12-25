#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <opencv2/opencv.hpp>

namespace Ui {
class MainWindow;
}
class ScreenRecorder;
class QThread;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
signals:
    void sigStartRecord(const QString &outputPath = QString());
    void sigStopRecord();
    void sigTestBool(bool a);
private slots:
    void on_loadImageBtn_clicked();
    void on_saveImageBtn_clicked();
    void on_btn_restore_clicked();
    void on_btn_exit_clicked();
    void on_startRecordBtn_clicked();
    void on_stopRecordBtn_clicked();
    // 基础色彩与通道操作
    void on_grayScaleBtn_clicked();          // 转换为灰度图
    void on_hsvBtn_clicked();                // 转换为HSV色彩空间
    void on_splitChannelsBtn_clicked();      // 分离与合并通道
    void on_brightnessContrastBtn_clicked(); // 调整亮度对比度

    // 滤波与平滑操作
//    void on_gaussianBlurBtn_clicked();       // 高斯模糊
    void on_medianBlurBtn_clicked();         // 中值滤波（去噪）
    void on_bilateralFilterBtn_clicked();    // 双边滤波（保边平滑）
    void on_boxFilterBtn_clicked();          // 均值滤波
    void on_sharpenBtn_clicked();            // 图像锐化

    // 形态学操作
    void on_erodeBtn_clicked();              // 腐蚀操作
    void on_dilateBtn_clicked();             // 膨胀操作
    void on_openBtn_clicked();               // 开运算（去噪点）
    void on_closeBtn_clicked();              // 闭运算（补孔洞）

    // 特征检测与描述
    void on_cannyBtn_clicked();              // Canny边缘检测
    void on_houghLinesBtn_clicked();         // 霍夫线检测
    void on_harrisCornerBtn_clicked();       // 角点检测（Harris）
//    void on_siftBtn_clicked();               // SIFT特征检测S

    // 图像分割
//    void on_thresholdBtn_clicked();          // 阈值化处理
    void on_adaptiveThresholdBtn_clicked();  // 自适应阈值化
    void on_kmeansBtn_clicked();             // K-means聚类分割
    void on_contourBtn_clicked();            // 轮廓检测

    // 几何变换操作
//    void on_resizeBtn_clicked();             // 图像缩放
//    void on_rotateBtn_clicked();             // 图像旋转

    // 其他基础操作
//    void on_invertBtn_clicked();             // 图像反转（负片效果）
//    void on_contrastBtn_clicked();           // 对比度增强（直方图均衡化）

    // 高阶操作
//    void on_stitchBtn_clicked();             // 图像拼接（全景图）
//    void on_bgSubtractionBtn_clicked();      // 背景减除（MOG2）
    void on_sobelBtn_clicked();              // Sobel边缘检测

private:
    std::shared_ptr<Ui::MainWindow> m_pUi;
    cv::Mat m_OrgImg;  // 原始图像
    cv::Mat m_ProcessedImg; // 处理后的图像
    std::shared_ptr<ScreenRecorder> m_pScreenRecorder;
    QThread* m_pRecordThread;
};

#endif // MAINWINDOW_H
