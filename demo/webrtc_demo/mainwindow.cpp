// mainwindow.cpp
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <modules/video_render/video_render.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // 初始化信令客户端（替换为实际信令服务器地址）
    signaling_ = new SignalingClient(QUrl("ws://localhost:8080"), this);
    // 初始化WebRTC
    webrtc_ = new WebRtcWrapper(signaling_, this);
    // 远程视频渲染组件
    remoteVideoWidget_ = new QVideoWidget(this);
    ui->remoteVideoLayout->addWidget(remoteVideoWidget_);
    // 绑定视频帧回调
    connect(webrtc_, &WebRtcWrapper::remoteVideoFrameReceived,
            this, &MainWindow::onRemoteVideoFrame);
}

MainWindow::~MainWindow() {
    delete ui;
    delete webrtc_;
    delete signaling_;
}

void MainWindow::on_startCallBtn_clicked() {
    webrtc_->startP2P();
}

void MainWindow::on_screenShareBtn_clicked() {
    webrtc_->startScreenShare();
}

void MainWindow::on_sendMp4Btn_clicked() {
    QString path = QFileDialog::getOpenFileName(this, "Select MP4", "", "MP4 Files (*.mp4)");
    if (!path.isEmpty()) {
        webrtc_->sendMP4Stream(path.toStdString());
    }
}

void MainWindow::onRemoteVideoFrame(const webrtc::VideoFrame& frame) {
    // 将WebRTC视频帧转为QImage并显示（简化：需处理格式转换）
    // 实际需用libyuv将I420转为RGB，再转为QImage
    qDebug() << "Received remote video frame, width:" << frame.width() << "height:" << frame.height();
}

