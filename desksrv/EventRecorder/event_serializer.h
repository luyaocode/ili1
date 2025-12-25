// event_serializer.h
#ifndef EVENT_SERIALIZER_H
#define EVENT_SERIALIZER_H

#include <QString>
#include <QList>
#include "recorded_event.h"

class EventSerializer {
public:
    static bool saveToFile(const QString& filename, const QList<RecordedEvent>& events);
    static bool loadFromFile(const QString& filename, QList<RecordedEvent>& events);
};

#endif // EVENT_SERIALIZER_H
