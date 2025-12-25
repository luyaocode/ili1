// mainwindow.h
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QVideoWidget>
#include "webrtcwrapper.h"
#include "signalingclient.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_startCallBtn_clicked();
    void on_screenShareBtn_clicked();
    void on_sendMp4Btn_clicked();
    void onRemoteVideoFrame(const webrtc::VideoFrame& frame);

private:
    Ui::MainWindow* ui;
    SignalingClient* signaling_;
    WebRtcWrapper* webrtc_;
    QVideoWidget* remoteVideoWidget_;
};

#endif // MAINWINDOW_H
