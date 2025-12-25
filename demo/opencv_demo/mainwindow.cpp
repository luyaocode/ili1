#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QThread>
#include "tool.h"
#include "screenrecorder.h"

// 设置标签固定宽高（示例：400x300，可根据需要调整）
constexpr const  int FIXED_WIDTH = 400;
constexpr const  int FIXED_HEIGHT = 300;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_pUi(std::shared_ptr<Ui::MainWindow>(new Ui::MainWindow))
{
    m_pUi->setupUi(this);
    m_pUi->originalLabel->setFixedSize(FIXED_WIDTH, FIXED_HEIGHT);
    m_pUi->originalLabel->setScaledContents(true); // 图像自动缩放适应标签
    m_pUi->processedLabel->setFixedSize(FIXED_WIDTH, FIXED_HEIGHT);
    m_pUi->processedLabel->setScaledContents(true); // 图像自动缩放适应标签
    setWindowTitle("OpenCV2 + Qt 演示");
    setFixedSize(size());
    m_pScreenRecorder = std::shared_ptr<ScreenRecorder>(new ScreenRecorder);
    m_pRecordThread = new QThread(this);
    m_pRecordThread->setObjectName("thread_record");
    m_pScreenRecorder->moveToThread(m_pRecordThread);
    connect(this,&MainWindow::sigStartRecord,m_pScreenRecorder.get(),&ScreenRecorder::startRecording);
    connect(this,&MainWindow::sigStopRecord,m_pScreenRecorder.get(),&ScreenRecorder::stopRecording);
    // 确保线程可以安全退出
    connect(m_pRecordThread, &QThread::finished, m_pScreenRecorder.get(), &QObject::deleteLater);
    connect(m_pRecordThread, &QThread::finished, m_pRecordThread, &QThread::deleteLater);
    connect(m_pScreenRecorder.get(),&ScreenRecorder::recordingStateChanged,this,[this](bool isRecording)
    {
        QString strText = isRecording?"录制中":"";
        m_pUi->label_record_stat->setText(strText);
        if(!isRecording)
        {
            m_pUi->label_screen_record->clear();
        }
    });
    connect(m_pScreenRecorder.get(),&ScreenRecorder::frameAvailable,this,[this](const cv::Mat &frame)
    {
        // 将cv::Mat转换为QImage
        QImage image = cvMatToQImage(frame);
        if (image.isNull()) return; // 防止空图像导致问题

        // 将QImage转换为QPixmap并按QLabel大小自动缩放
        QPixmap pixmap = QPixmap::fromImage(image);

        // 缩放图像以适应QLabel，保持比例并填充整个空间
        QPixmap scaledPixmap = pixmap.scaled(
            m_pUi->label_screen_record->size(),  // 目标大小（QLabel的当前大小）
            Qt::KeepAspectRatio,                 // 保持宽高比
            Qt::SmoothTransformation             // 平滑缩放，画质更好
        );

        // 设置缩放后的图像到QLabel
        m_pUi->label_screen_record->setPixmap(scaledPixmap);
    });

    // 当主窗口销毁时停止线程
    connect(this, &MainWindow::destroyed, this, [this]() {
        m_pRecordThread->quit();
        m_pRecordThread->wait();
    });
    m_pRecordThread->start();
}

MainWindow::~MainWindow()
{
}

// 加载图像
void MainWindow::on_loadImageBtn_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this,
        tr("选择图像"), "", tr("图像文件 (*.png *.jpg *.bmp *.jpeg)"));

    if (filePath.isEmpty())
        return;

    // 使用 OpenCV 加载图像
    m_OrgImg = cv::imread(filePath.toStdString());
    if (m_OrgImg.empty())
    {
        QMessageBox::warning(this, tr("错误"), tr("无法加载图像!"));
        return;
    }

    // 显示原始图像
    m_ProcessedImg = m_OrgImg.clone();

    m_pUi->originalLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_OrgImg)));
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));

    m_pUi->originalLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 转换为灰度图
void MainWindow::on_grayScaleBtn_clicked()
{
    if (m_OrgImg.empty())
    {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }

    // 转换为灰度图
    cv::cvtColor(m_OrgImg, m_ProcessedImg, CV_BGR2GRAY);

    // 显示处理后的图像
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// Canny 边缘检测
void MainWindow::on_cannyBtn_clicked()
{
    if (m_OrgImg.empty())
    {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }

    cv::Mat grayMat, edgeMat;

    // 先转换为灰度图
    cv::cvtColor(m_OrgImg, grayMat, CV_BGR2GRAY);
    // 高斯模糊降噪
    cv::GaussianBlur(grayMat, grayMat, cv::Size(3, 3), 0);
    // Canny 边缘检测
    cv::Canny(grayMat, edgeMat, 50, 150);

    m_ProcessedImg = edgeMat;
    // 显示边缘检测结果
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 保存处理后的图像
void MainWindow::on_saveImageBtn_clicked()
{
    if (m_ProcessedImg.empty())
    {
        QMessageBox::warning(this, tr("警告"), tr("没有可保存的图像!"));
        return;
    }
    QString filter;
    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("保存图像"),
        "",
        tr("PNG 图像 (*.png);;JPEG 图像 (*.jpg);;BMP 图像 (*.bmp)"),
        &filter // 获取用户选择的过滤器
    );

    if (filePath.isEmpty())
        return;

    // 从选择的过滤器中提取扩展名
    QString extension;
    if (filter.contains("(*.png)")) {
        extension = ".png";
    } else if (filter.contains("(*.jpg)")) {
        extension = ".jpg";
    } else if (filter.contains("(*.bmp)")) {
        extension = ".bmp";
    }

    // 检查文件是否已包含扩展名，如果没有则添加
    QFileInfo fileInfo(filePath);
    if (fileInfo.suffix().isEmpty())
    {
        filePath += extension;
    }

    // 为imwrite添加异常捕获
    try
    {
        // 尝试保存图像
        bool saveSuccess = cv::imwrite(filePath.toStdString(), m_ProcessedImg);

        if (!saveSuccess) {
            throw std::runtime_error("OpenCV无法写入图像文件，可能是格式不支持或权限问题");
        }

        QMessageBox::information(this, tr("成功"), tr("图像已保存至：\n%1").arg(filePath));
    }
    catch (const cv::Exception& e) {
        QMessageBox::critical(this, tr("OpenCV错误"),
            tr("保存失败：%1").arg(QString::fromStdString(e.what())));
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, tr("保存错误"),
            tr("保存失败：%1").arg(QString::fromStdString(e.what())));
    }
    catch (...) {
        QMessageBox::critical(this, tr("未知错误"), tr("保存图像时发生未知错误"));
    }
}

void MainWindow::on_sharpenBtn_clicked()
{
    if (m_OrgImg.empty())
    {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    // 定义一个锐化卷积核 (拉普拉斯算子)
    cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
                      -1, -1, -1,
                      -1,  9, -1, // 中心值从 5 增加到 9
                      -1, -1, -1);
    // 使用filter2D进行卷积
    cv::filter2D(m_OrgImg, m_ProcessedImg, m_OrgImg.depth(), kernel);
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}


void MainWindow::on_btn_restore_clicked()
{
    m_ProcessedImg = m_OrgImg.clone();
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

void MainWindow::on_btn_exit_clicked()
{
    close();
}

void MainWindow::on_startRecordBtn_clicked()
{
    emit sigStartRecord();
}

void MainWindow::on_stopRecordBtn_clicked()
{
    emit sigStopRecord();
}


// 1. 转换为HSV色彩空间
void MainWindow::on_hsvBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::cvtColor(m_OrgImg, m_ProcessedImg, cv::COLOR_BGR2HSV);
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 2. 分离与合并通道
void MainWindow::on_splitChannelsBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    std::vector<cv::Mat> channels;
    cv::split(m_OrgImg, channels);
    // 仅显示蓝色通道（BGR顺序）
    m_ProcessedImg = channels[0].clone();
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 3. 调整亮度对比度
void MainWindow::on_brightnessContrastBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    double alpha = 1.5;  // 对比度增益
    int beta = 50;       // 亮度偏移
    m_OrgImg.convertTo(m_ProcessedImg, -1, alpha, beta);
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 1. 中值滤波（去噪）
void MainWindow::on_medianBlurBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::medianBlur(m_OrgImg, m_ProcessedImg, 5);  // 5x5内核
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 2. 双边滤波（保边平滑）
void MainWindow::on_bilateralFilterBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::bilateralFilter(m_OrgImg, m_ProcessedImg, 9, 75, 75);
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 3. 均值滤波
void MainWindow::on_boxFilterBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::boxFilter(m_OrgImg, m_ProcessedImg, -1, cv::Size(5, 5));
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 1. 腐蚀操作
void MainWindow::on_erodeBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::erode(m_OrgImg, m_ProcessedImg, kernel);
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 2. 膨胀操作
void MainWindow::on_dilateBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(m_OrgImg, m_ProcessedImg, kernel);
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 3. 开运算（去噪点）
void MainWindow::on_openBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(m_OrgImg, m_ProcessedImg, cv::MORPH_OPEN, kernel);
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 4. 闭运算（补孔洞）
void MainWindow::on_closeBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(m_OrgImg, m_ProcessedImg, cv::MORPH_CLOSE, kernel);
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 1. 霍夫线检测
void MainWindow::on_houghLinesBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::Mat gray, edges;
    cv::cvtColor(m_OrgImg, gray, cv::COLOR_BGR2GRAY);
    cv::Canny(gray, edges, 50, 150);

    std::vector<cv::Vec2f> lines;
    cv::HoughLines(edges, lines, 1, CV_PI/180, 100);

    m_ProcessedImg = m_OrgImg.clone();
    for (size_t i = 0; i < lines.size(); i++) {
        float rho = lines[i][0], theta = lines[i][1];
        cv::Point pt1, pt2;
        double a = cos(theta), b = sin(theta);
        double x0 = a*rho, y0 = b*rho;
        pt1.x = cvRound(x0 + 1000*(-b));
        pt1.y = cvRound(y0 + 1000*(a));
        pt2.x = cvRound(x0 - 1000*(-b));
        pt2.y = cvRound(y0 - 1000*(a));
        cv::line(m_ProcessedImg, pt1, pt2, cv::Scalar(0,0,255), 2);
    }
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 2. 角点检测（Harris）
void MainWindow::on_harrisCornerBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::Mat gray, dst, dst_norm, dst_norm_scaled;
    cv::cvtColor(m_OrgImg, gray, cv::COLOR_BGR2GRAY);
    dst = cv::Mat::zeros(gray.size(), CV_32FC1);

    cv::cornerHarris(gray, dst, 2, 3, 0.04);
    cv::normalize(dst, dst_norm, 0, 255, cv::NORM_MINMAX, CV_32FC1, cv::Mat());
    cv::convertScaleAbs(dst_norm, dst_norm_scaled);

    m_ProcessedImg = m_OrgImg.clone();
    for (int j = 0; j < dst_norm.rows; j++) {
        for (int i = 0; i < dst_norm.cols; i++) {
            if ((int)dst_norm.at<float>(j,i) > 100) {
                cv::circle(m_ProcessedImg, cv::Point(i,j), 5, cv::Scalar(0,0,255), 2);
            }
        }
    }
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 3. SIFT特征检测（需编译带非自由模块的OpenCV）
//void MainWindow::on_siftBtn_clicked()
//{
//    if (m_OrgImg.empty()) {
//        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
//        return;
//    }
//    cv::Mat gray;
//    cv::cvtColor(m_OrgImg, gray, cv::COLOR_BGR2GRAY);

//    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
//    std::vector<cv::KeyPoint> keypoints;
//    cv::Mat descriptors;
//    sift->detectAndCompute(gray, cv::Mat(), keypoints, descriptors);

//    m_ProcessedImg = m_OrgImg.clone();
//    cv::drawKeypoints(m_ProcessedImg, keypoints, m_ProcessedImg,
//                     cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
//    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
//}

// 1. 阈值化（自适应阈值）
void MainWindow::on_adaptiveThresholdBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::Mat gray;
    cv::cvtColor(m_OrgImg, gray, cv::COLOR_BGR2GRAY);
    cv::adaptiveThreshold(gray, m_ProcessedImg, 255,
                         cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                         cv::THRESH_BINARY, 11, 2);
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 2. K-means聚类分割
void MainWindow::on_kmeansBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::Mat reshaped = m_OrgImg.reshape(1, m_OrgImg.total());
    reshaped.convertTo(reshaped, CV_32F);

    cv::Mat labels, centers;
    int k = 3;  // 聚类数
    cv::kmeans(reshaped, k, labels,
              cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 10, 1.0),
              3, cv::KMEANS_PP_CENTERS, centers);

    centers.convertTo(centers, CV_8UC1);
    cv::Mat processed(m_OrgImg.size(), CV_8UC3);
    for (int i = 0; i < m_OrgImg.rows; ++i) {
        for (int j = 0; j < m_OrgImg.cols; ++j) {
            int label = labels.at<int>(i, j);
            processed.at<cv::Vec3b>(i, j) = centers.row(label).clone().reshape(3, 1).at<cv::Vec3b>(0, 0);
        }
    }
    m_ProcessedImg = processed;
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 3. 轮廓检测
void MainWindow::on_contourBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::Mat gray, binary;
    cv::cvtColor(m_OrgImg, gray, cv::COLOR_BGR2GRAY);
    cv::threshold(gray, binary, 127, 255, cv::THRESH_BINARY);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    m_ProcessedImg = m_OrgImg.clone();
    cv::drawContours(m_ProcessedImg, contours, -1, cv::Scalar(0,255,0), 2);
    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}

// 1. 图像拼接（全景图）
//void MainWindow::on_stitchBtn_clicked()
//{
//    // 注意：需要至少两张图像，这里假设有m_OrgImg和m_SecondImg
//    if (m_OrgImg.empty() || m_SecondImg.empty()) {
//        QMessageBox::warning(this, tr("警告"), tr("请加载两张图像!"));
//        return;
//    }
//    std::vector<cv::Mat> imgs = {m_OrgImg, m_SecondImg};
//    cv::Ptr<cv::Stitcher> stitcher = cv::Stitcher::create();
//    cv::Stitcher::Status status = stitcher->stitch(imgs, m_ProcessedImg);

//    if (status != cv::Stitcher::OK) {
//        QMessageBox::warning(this, tr("警告"), tr("拼接失败!"));
//        return;
//    }
//    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
//}

// 2. 背景减除（MOG2）
//void MainWindow::on_bgSubtractionBtn_clicked()
//{
//    // 注意：通常用于视频，这里用单张图像示例
//    if (m_OrgImg.empty() || m_BgImg.empty()) {  // m_BgImg为背景图像
//        QMessageBox::warning(this, tr("警告"), tr("请加载图像和背景!"));
//        return;
//    }
//    cv::Ptr<cv::BackgroundSubtractorMOG2> fgbg = cv::createBackgroundSubtractorMOG2();
//    cv::Mat fgmask;
//    fgbg->apply(m_BgImg, fgmask);  // 先传入背景
//    fgbg->apply(m_OrgImg, fgmask); // 再传入前景

//    m_ProcessedImg = fgmask.clone();
//    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
//}

// 3. 边缘检测（Sobel算子）
void MainWindow::on_sobelBtn_clicked()
{
    if (m_OrgImg.empty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先加载图像!"));
        return;
    }
    cv::Mat gray, grad_x, grad_y, abs_grad_x, abs_grad_y;
    cv::cvtColor(m_OrgImg, gray, cv::COLOR_BGR2GRAY);

    cv::Sobel(gray, grad_x, CV_16S, 1, 0, 3);
    cv::Sobel(gray, grad_y, CV_16S, 0, 1, 3);

    cv::convertScaleAbs(grad_x, abs_grad_x);
    cv::convertScaleAbs(grad_y, abs_grad_y);
    cv::addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, m_ProcessedImg);

    m_pUi->processedLabel->setPixmap(QPixmap::fromImage(cvMatToQImage(m_ProcessedImg)));
}
