// x11_event_player.cpp
#include "x11_event_player.h"
#include <QDebug>
#include <QThread>
#include <QDateTime>
#include "x11struct.h"

// 手动声明 XTest 核心函数（因缺少 XTest.h，直接声明函数原型）
extern "C" {
void XTestFakeKeyEvent(Display *dpy, unsigned int keycode, Bool press, unsigned long delay);
Bool XTestQueryExtension(Display *display, int *event_base_return, int *error_base_return);
void XTestFakeButtonEvent(
    void* display,
    unsigned int button,
    int press,
    unsigned long delay
);
}


X11EventPlayer::X11EventPlayer(QObject *parent)
    : QObject(parent), display(nullptr) {}

X11EventPlayer::~X11EventPlayer() {
    cleanupX11();
}

bool X11EventPlayer::initializeX11() {
    display = XOpenDisplay(nullptr);
    if (!display) {
        emit errorOccurred("Failed to open X display.");
        return false;
    }
    // 检查 XTest 扩展是否可用
    int xtest_event_base, xtest_error_base;
    if (!XTestQueryExtension(display, &xtest_event_base, &xtest_error_base)) {
        emit errorOccurred("XTest extension not available. Cannot simulate events.");
        return false;
    }
    return true;
}

void X11EventPlayer::cleanupX11() {
    // XCloseDisplay is managed by Qt
    display = nullptr;
}

bool X11EventPlayer::playEvents(const QList<RecordedEvent>& events) {
    if (events.isEmpty()) {
        emit errorOccurred("No events to play.");
        return false;
    }

    if (!initializeX11()) {
        return false;
    }

    emit playbackStarted();
    qInfo() << "Starting playback of" << events.size() << "events...";

    qint64 playbackStartTime = QDateTime::currentMSecsSinceEpoch();

    for (int i = 0; i < events.size() && display; ++i) {
        const RecordedEvent& event = events[i];

        // 计算需要等待的时间，以保证事件的时间间隔与记录时一致
        qint64 expectedPlaybackTime = playbackStartTime + event.timestamp;
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        qint64 sleepTime = expectedPlaybackTime - currentTime;

        if (sleepTime > 0) {
            QThread::msleep(sleepTime);
        }

        if (!simulateEvent(event)) {
            qWarning() << "Failed to simulate event at index" << i;
        }

        emit playbackProgress(i + 1, events.size());
    }

    emit playbackFinished();
    qInfo() << "Playback finished.";
    cleanupX11();
    return true;
}

bool X11EventPlayer::simulateEvent(const RecordedEvent& event) {
    if (!display) return false;

    switch (event.type) {
        case EventType::MOUSE_MOVE:
            XWarpPointer(display, None, None, 0, 0, 0, 0, event.x, event.y);
            break;
        case EventType::MOUSE_PRESS:
            XTestFakeButtonEvent(display, event.button, True, CurrentTime);
            break;
        case EventType::MOUSE_RELEASE:
            XTestFakeButtonEvent(display, event.button, False, CurrentTime);
            break;
        case EventType::KEY_PRESS: {
            KeySym keysym = event.keySym;
            if (keysym == NoSymbol) {
                qWarning() << "Unknown KeySym:" << event.keySym;
                return false;
            }
            KeyCode keycode = XKeysymToKeycode(display, keysym);
            if (keycode == 0) {
                qWarning() << "KeySym" << event.keySym << "has no corresponding KeyCode.";
                return false;
            }
            XTestFakeKeyEvent(display, keycode, True, CurrentTime);
            break;
        }
        case EventType::KEY_RELEASE: {
            KeySym keysym = event.keySym;
            if (keysym == NoSymbol) {
                qWarning() << "Unknown KeySym:" << event.keySym;
                return false;
            }
            KeyCode keycode = XKeysymToKeycode(display, keysym);
            if (keycode == 0) {
                qWarning() << "KeySym" << event.keySym << "has no corresponding KeyCode.";
                return false;
            }
            XTestFakeKeyEvent(display, keycode, False, CurrentTime);
            break;
        }
        default:
            qWarning() << "Cannot simulate unknown event type.";
            return false;
    }

    XFlush(display); // 确保事件被立即发送
    return true;
}
