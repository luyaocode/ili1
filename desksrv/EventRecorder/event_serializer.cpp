// event_serializer.cpp
#include "event_serializer.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include "x11struct.h"

bool EventSerializer::saveToFile(const QString& filename, const QList<RecordedEvent>& events) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qCritical() << "Failed to open file for writing:" << filename;
        return false;
    }

    QTextStream out(&file);
    out << "# X11 Global Event Recording\n";
    out << "# Format: Type Timestamp(ms) [Params...]\n";
    out << "# Types: MOUSE_MOVE, MOUSE_PRESS, MOUSE_RELEASE, KEY_PRESS, KEY_RELEASE\n";
    out << "# Mouse Params: X Y\n";
    out << "# Button Params: ButtonCode (1=Left, 2=Middle, 3=Right...)\n";
    out << "# Key Params: KeySym (e.g., a, A, Return, Control_L)\n";

    for (const auto& event : events) {
        out << eventTypeToString(event.type) << " " << event.timestamp << " ";
        switch (event.type) {
            case EventType::MOUSE_MOVE:
                out << event.x << " " << event.y;
                break;
            case EventType::MOUSE_PRESS:
            case EventType::MOUSE_RELEASE:
                out << event.button;
                break;
            case EventType::KEY_PRESS:
            case EventType::KEY_RELEASE:
                out << event.keySym;
                break;
            default:
                break;
        }
        out << "\n";
    }

    file.close();
    qInfo() << "Successfully saved" << events.size() << "events to" << filename;
    return true;
}

bool EventSerializer::loadFromFile(const QString& filename, QList<RecordedEvent>& events) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCritical() << "Failed to open file for reading:" << filename;
        return false;
    }

    QTextStream in(&file);
    QString line;
    events.clear();

    while (in.readLineInto(&line)) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith('#')) {
            continue;
        }

        QStringList parts = line.split(' ', QString::SkipEmptyParts);
        if (parts.size() < 2) {
            qWarning() << "Skipping malformed line:" << line;
            continue;
        }

        RecordedEvent event;
        event.type = stringToEventType(parts[0]);
        event.timestamp = parts[1].toLongLong();

        if (event.type == EventType::UNKNOWN) {
            qWarning() << "Skipping unknown event type:" << parts[0];
            continue;
        }

        bool paramsValid = true;
        switch (event.type) {
            case EventType::MOUSE_MOVE:
                if (parts.size() >= 4) {
                    event.x = parts[2].toInt(&paramsValid);
                    event.y = parts[3].toInt(&paramsValid);
                } else paramsValid = false;
                break;
            case EventType::MOUSE_PRESS:
            case EventType::MOUSE_RELEASE:
                if (parts.size() >= 3) {
                    event.button = parts[2].toInt(&paramsValid);
                } else paramsValid = false;
                break;
            case EventType::KEY_PRESS:
            case EventType::KEY_RELEASE:
                if (parts.size() >= 3) {
                    event.keySym = XStringToKeysym(parts[2].toStdString().c_str());
                } else paramsValid = false;
                break;
            default:
                paramsValid = false;
                break;
        }

        if (paramsValid) {
            events.append(event);
        } else {
            qWarning() << "Skipping line with invalid parameters:" << line;
        }
    }

    file.close();
    qInfo() << "Successfully loaded" << events.size() << "events from" << filename;
    return true;
}
