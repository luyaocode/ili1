// webrtc_wrapper.h
#ifndef WEBRTC_WRAPPER_H
#define WEBRTC_WRAPPER_H

#include <QObject>
#include <api/peer_connection_interface.h>
#include <api/media_stream_interface.h>
#include <modules/desktop_capture/desktop_capturer.h>
#include <modules/video_capture/video_capture.h>


class SignalingClient;

// 转发 WebRTC 日志到 Qt 控制台
class QtLogSink : public rtc::LogSink {
public:
    void OnLogMessage(const std::string& message) override {
        qDebug() << "[WebRTC] " << message.c_str();
    }
};

class WebRtcWrapper : public QObject,
                      public webrtc::PeerConnectionObserver,
                      public webrtc::CreateSessionDescriptionObserver {
//    Q_OBJECT
public:
    explicit WebRtcWrapper(SignalingClient* signaling, QObject* parent = nullptr);
    ~WebRtcWrapper() override;

    void startP2P(); // 初始化P2P连接
    void startScreenShare(); // 开启屏幕共享
    void sendMP4Stream(const std::string& mp4Path); // 发送MP4流

signals:
    void remoteVideoFrameReceived(const webrtc::VideoFrame& frame); // 供Qt渲染

public slots:
    void onSignalingMessage(const std::string& msg); // 处理收到的信令消息

private:
    // PeerConnectionObserver 接口（P2P连接状态回调）
    void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {}
    void OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override;
    void OnRemoveStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) override {}
    void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
    void OnRenegotiationNeeded() override;
    void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override;
    void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
    void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;

    // CreateSessionDescriptionObserver 接口（SDP生成回调）
    void OnSuccess(rtc::scoped_refptr<webrtc::SessionDescriptionInterface> desc) override;
    void OnFailure(webrtc::RTCError error) override {
        qWarning() << "SDP create failed:" << error.message();
    }

    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_factory_;
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
    SignalingClient* signaling_; // 信令客户端指针
    QtLogSink log_sink_; // 日志输出
    rtc::scoped_refptr<webrtc::VideoTrackInterface> screen_track_; // 屏幕共享轨道
};

#endif // WEBRTC_WRAPPER_H
