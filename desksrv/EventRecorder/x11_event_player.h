// x11_event_player.h
#ifndef X11_EVENT_PLAYER_H
#define X11_EVENT_PLAYER_H

#include <QObject>
#include <QList>
#include "recorded_event.h"

typedef struct _XDisplay Display;

class X11EventPlayer : public QObject {
    Q_OBJECT
public:
    explicit X11EventPlayer(QObject *parent = nullptr);
    ~X11EventPlayer();

    bool playEvents(const QList<RecordedEvent>& events);

signals:
    void playbackStarted();
    void playbackFinished();
    void playbackProgress(int current, int total);
    void errorOccurred(const QString& errorMessage);

private:
    Display* display;
    bool initializeX11();
    void cleanupX11();
    bool simulateEvent(const RecordedEvent& event);
};

#endif // X11_EVENT_PLAYER_H
