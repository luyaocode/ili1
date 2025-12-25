// webrtc_wrapper.cpp
#include "webrtcwrapper.h"
#include "signalingclient.h"
#include <QDebug>
#include <rtc_base/ssl_adapter.h>
#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/video_codecs/builtin_video_decoder_factory.h>
#include <api/video_codecs/builtin_video_encoder_factory.h>
#include <modules/desktop_capture/desktop_capturer.h>
#include <modules/desktop_capture/desktop_frame.h>
#include <modules/desktop_capture/desktop_capture_options.h>

WebRtcWrapper::WebRtcWrapper(SignalingClient* signaling, QObject* parent)
    : QObject(parent), signaling_(signaling) {
    // 初始化WebRTC日志
    rtc::LogMessage::AddLogToStream(&log_sink_, rtc::LS_INFO);
    // 初始化SSL（信令加密需要）
    rtc::InitializeSSL();
    // 绑定信令消息回调
    connect(signaling_, &SignalingClient::messageReceived,
            this, &WebRtcWrapper::onSignalingMessage);
}

WebRtcWrapper::~WebRtcWrapper() {
    rtc::CleanupSSL();
    rtc::LogMessage::RemoveLogFromStream(&log_sink_);
}

void WebRtcWrapper::startP2P() {
    // 1. 创建PeerConnection工厂
    peer_factory_ = webrtc::CreatePeerConnectionFactory(
        nullptr, nullptr, nullptr, nullptr,
        webrtc::CreateBuiltinAudioEncoderFactory(),
        webrtc::CreateBuiltinAudioDecoderFactory(),
        webrtc::CreateBuiltinVideoEncoderFactory(),
        webrtc::CreateBuiltinVideoDecoderFactory(),
        nullptr, nullptr);

    // 2. 配置STUN服务器（NAT穿透）
    webrtc::PeerConnectionInterface::RTCConfiguration config;
    webrtc::PeerConnectionInterface::IceServer server;
    server.uri = "stun:stun.l.google.com:19302"; // 谷歌公共STUN
    config.servers.push_back(server);

    // 3. 创建PeerConnection
    peer_connection_ = peer_factory_->CreatePeerConnection(
        config, nullptr, nullptr, this);

    // 4. 创建并添加本地音视频流（简化：仅添加视频）
    auto video_source = peer_factory_->CreateVideoSource(
        webrtc::VideoCaptureModule::DeviceInfo::Create()->CreateDevice(
            webrtc::VideoCaptureModule::DeviceInfo::GetDeviceName(0).c_str()),
        nullptr);
    auto video_track = peer_factory_->CreateVideoTrack("video_track", video_source);
    auto stream = peer_factory_->CreateLocalMediaStream("local_stream");
    stream->AddTrack(video_track);
    peer_connection_->AddStream(stream);

    // 5. 发起offer（作为呼叫方）
    peer_connection_->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void WebRtcWrapper::startScreenShare() {
    // 创建屏幕捕获器（Linux X11）
    auto capturer = webrtc::DesktopCapturer::CreateScreenCapturer(
        webrtc::DesktopCaptureOptions::CreateDefault());
    // 封装为WebRTC视频轨道（简化：需实现VideoSourceInterface接口）
    // 实际需循环捕获屏幕帧并通过track发送，此处仅示意
    screen_track_ = peer_factory_->CreateVideoTrack("screen_track", nullptr);
    auto stream = peer_factory_->CreateLocalMediaStream("screen_stream");
    stream->AddTrack(screen_track_);
    peer_connection_->AddStream(stream);
    // 触发重新协商
    peer_connection_->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void WebRtcWrapper::sendMP4Stream(const std::string& mp4Path) {
    // 实际需用FFmpeg解码MP4为H.264帧，再通过VideoTrack发送
    // 示例：创建自定义VideoSource，循环读取帧并推送
    qDebug() << "Sending MP4 stream from:" << mp4Path.c_str();
    // 简化：仅打印路径，实际需实现帧解码和发送逻辑
}

void WebRtcWrapper::OnAddStream(rtc::scoped_refptr<webrtc::MediaStreamInterface> stream) {
    // 收到远程媒体流，绑定视频帧回调
    auto video_track = stream->GetVideoTracks()[0];
    video_track->AddOrUpdateSink(this, rtc::VideoSinkWants()); // 需实现VideoSinkInterface
}

void WebRtcWrapper::OnRenegotiationNeeded() {
    // 需要重新协商（如添加新流时）
    peer_connection_->CreateOffer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void WebRtcWrapper::OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) {
    qDebug() << "ICE connection state:" << new_state;
    if (new_state == webrtc::PeerConnectionInterface::kIceConnectionConnected) {
        qDebug() << "P2P connected!";
    }
}

void WebRtcWrapper::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
    // 生成ICE候选消息，通过信令发送给对端
    std::string sdp;
    candidate->ToString(&sdp);
    signaling_->sendMessage("{\"type\":\"candidate\", \"sdp\":" + sdp + "}");
}
void WebRtcWrapper::OnSuccess(rtc::scoped_refptr<webrtc::SessionDescriptionInterface> desc) {
    // SDP生成成功，设置本地描述并发送给对端
    peer_connection_->SetLocalDescription(webrtc::DummySetSessionDescriptionObserver::Create(), desc);
    std::string sdp;
    desc->ToString(&sdp);
    signaling_->sendMessage("{\"type\":\"" + desc->type() + "\", \"sdp\":" + sdp + "}");
}

void WebRtcWrapper::onSignalingMessage(const std::string& msg) {
    // 解析信令消息（简化：实际需用JSON库解析）
    qDebug() << "Received signaling message:" << msg.c_str();
    if (msg.find("offer") != std::string::npos) {
        // 收到offer，创建answer
        auto desc = webrtc::CreateSessionDescription("offer", msg);
        peer_connection_->SetRemoteDescription(
            webrtc::DummySetSessionDescriptionObserver::Create(), desc);
        peer_connection_->CreateAnswer(this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
    } else if (msg.find("answer") != std::string::npos) {
        // 收到answer，设置远程描述
        auto desc = webrtc::CreateSessionDescription("answer", msg);
        peer_connection_->SetRemoteDescription(
            webrtc::DummySetSessionDescriptionObserver::Create(), desc);
    } else if (msg.find("candidate") != std::string::npos) {
        // 收到ICE候选，添加到连接
        webrtc::SdpParseError error;
        auto candidate = webrtc::CreateIceCandidate("", 0, msg, &error);
        if (candidate) {
            peer_connection_->AddIceCandidate(candidate);
        }
    }
}
