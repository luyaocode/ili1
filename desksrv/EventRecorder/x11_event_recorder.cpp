// x11_event_recorder.cpp
#include "x11_event_recorder.h"
#include <QDebug>
#include <QX11Info> // For QX11Info::display() (requires Qt >= 5.0)
#include <unistd.h> // for usleep
#include <QDateTime>
#include "x11struct.h"

X11EventRecorder::X11EventRecorder(QObject *parent)
    : QObject(parent), display(nullptr), rootWindow(None), isRecording(false) {
    moveToThread(&workerThread);
    connect(&workerThread, &QThread::started, this, &X11EventRecorder::runEventLoop);
    connect(&workerThread, &QThread::finished, this, &X11EventRecorder::deleteLater);
}

X11EventRecorder::~X11EventRecorder() {
    stopRecording();
    workerThread.quit();
    workerThread.wait();
    cleanupX11();
}

bool X11EventRecorder::initializeX11() {
    display = XOpenDisplay(nullptr);
    if (!display) {
        emit errorOccurred("Failed to open X display.");
        return false;
    }

    rootWindow = DefaultRootWindow(display);
    if (rootWindow == None) {
        emit errorOccurred("Failed to get root window.");
        cleanupX11();
        return false;
    }

    return true;
}

void X11EventRecorder::cleanupX11() {
    if (display) {
        // 如果正在记录，需要取消选择事件，否则会导致其他应用出问题
        if (isRecording && rootWindow != None) {
             XSelectInput(display, rootWindow, 0); // 取消所有事件选择
             XFlush(display);
        }
        // 注意：不要调用 XCloseDisplay，因为 QX11Info::display() 返回的连接由 Qt 管理
        // 如果是我们自己用 XOpenDisplay 打开的，则需要关闭。这里为了简化，假设由 Qt 管理。
        // if (display != QX11Info::display()) XCloseDisplay(display);
        display = nullptr;
    }
    rootWindow = None;
}

bool X11EventRecorder::startRecording() {
    if (isRecording) return true;

    if (!initializeX11()) {
        return false;
    }

    // 选择我们感兴趣的事件
    unsigned long eventMask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask;
    XSelectInput(display, rootWindow, eventMask);
    XFlush(display); // 确保请求被发送

    recordedEvents.clear();
    recordingStartTime = QDateTime::currentMSecsSinceEpoch();
    isRecording = true;

    workerThread.start();
    emit recordingStarted();
    qInfo() << "X11 Event Recorder started.";
    return true;
}

void X11EventRecorder::stopRecording() {
    if (!isRecording) return;

    isRecording = false;
    // 中断 XNextEvent 的阻塞
    if (display) {
        XFlush(display);
        // 在某些情况下，可能需要更强制的手段，比如发送一个客户端消息给自己
        // 但对于简单的停止来说，设置 isRecording 并等待线程退出通常就足够了
    }

    workerThread.quit();
    workerThread.wait();

    cleanupX11();
    emit recordingStopped();
    qInfo() << "X11 Event Recorder stopped.";
}

QList<RecordedEvent> X11EventRecorder::getRecordedEvents() const {
    return recordedEvents;
}

void X11EventRecorder::runEventLoop() {
    if (!display || !isRecording) {
        emit errorOccurred("Event loop aborted: Display not initialized or not recording.");
        workerThread.quit();
        return;
    }

    XEvent event;
    while (isRecording) {
        // XNextEvent 是阻塞的，直到有事件可用
        if (XPending(display) > 0) {
             XNextEvent(display, &event);
             processXEvent(event);
        } else {
             // 避免 CPU 占用过高，短暂休眠
             usleep(1000); // 1ms
        }
    }
}

void X11EventRecorder::processXEvent(XEvent& event) {
    if (!isRecording) return;

    RecordedEvent recEvent;
    recEvent.timestamp = QDateTime::currentMSecsSinceEpoch() - recordingStartTime;

    switch (event.type) {
        case MotionNotify:
            recEvent.type = EventType::MOUSE_MOVE;
            recEvent.x = event.xmotion.x_root;
            recEvent.y = event.xmotion.y_root;
            break;
        case ButtonPress:
            recEvent.type = EventType::MOUSE_PRESS;
            recEvent.button = event.xbutton.button;
            break;
        case ButtonRelease:
            recEvent.type = EventType::MOUSE_RELEASE;
            recEvent.button = event.xbutton.button;
            break;
        case KeyPress: {
            recEvent.type = EventType::KEY_PRESS;
            KeySym keysym = XLookupKeysym(&event.xkey, 0);
            recEvent.keySym = keysym;
            break;
        }
        case KeyRelease: {
            recEvent.type = EventType::KEY_RELEASE;
            KeySym keysym = XLookupKeysym(&event.xkey, 0);
            recEvent.keySym = keysym;
            break;
        }
        default:
            // 忽略其他类型的事件
            return;
    }

    recordedEvents.append(recEvent);
    // qDebug() << "Recorded:" << eventTypeToString(recEvent.type) << recEvent.timestamp;
}
