#ifndef RECORDED_EVENT_H
#define RECORDED_EVENT_H

#include <QString>

enum class EventType {
    UNKNOWN,
    MOUSE_MOVE,
    MOUSE_PRESS,
    MOUSE_RELEASE,
    KEY_PRESS,
    KEY_RELEASE
};

inline QString eventTypeToString(EventType type) {
    switch (type) {
        case EventType::MOUSE_MOVE: return "MOUSE_MOVE";
        case EventType::MOUSE_PRESS: return "MOUSE_PRESS";
        case EventType::MOUSE_RELEASE: return "MOUSE_RELEASE";
        case EventType::KEY_PRESS: return "KEY_PRESS";
        case EventType::KEY_RELEASE: return "KEY_RELEASE";
        default: return "UNKNOWN";
    }
}

inline EventType stringToEventType(const QString& str) {
    if (str == "MOUSE_MOVE") return EventType::MOUSE_MOVE;
    if (str == "MOUSE_PRESS") return EventType::MOUSE_PRESS;
    if (str == "MOUSE_RELEASE") return EventType::MOUSE_RELEASE;
    if (str == "KEY_PRESS") return EventType::KEY_PRESS;
    if (str == "KEY_RELEASE") return EventType::KEY_RELEASE;
    return EventType::UNKNOWN;
}

struct RecordedEvent {
    EventType type;
    qint64 timestamp; // 相对时间，毫秒

    // 联合使用，节省空间
    union {
        struct { int x, y; };          // for MOUSE_MOVE
        int button;                    // for MOUSE_PRESS/RELEASE (1=Left, 2=Middle, 3=Right)
        unsigned long long  keySym;                // for KEY_PRESS/RELEASE (e.g., "a", "Return", "Control_L")
    };
};

#endif // RECORDED_EVENT_H
