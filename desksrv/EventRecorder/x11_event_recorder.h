// x11_event_recorder.h
#ifndef X11_EVENT_RECORDER_H
#define X11_EVENT_RECORDER_H

#include <QObject>
#include <QThread>
#include <QList>
#include "recorded_event.h"

// Forward declarations for XLib
typedef struct _XDisplay Display;
typedef unsigned long Window;
typedef union _XEvent XEvent;

class X11EventRecorder : public QObject {
    Q_OBJECT
public:
    explicit X11EventRecorder(QObject *parent = nullptr);
    ~X11EventRecorder();

    bool startRecording();
    void stopRecording();
    QList<RecordedEvent> getRecordedEvents() const;

signals:
    void recordingStarted();
    void recordingStopped();
    void errorOccurred(const QString& errorMessage);

private slots:
    void runEventLoop(); // 事件循环将在工作线程中运行

private:
    Display* display;
    Window rootWindow;
    QThread workerThread;
    bool isRecording;
    QList<RecordedEvent> recordedEvents;
    qint64 recordingStartTime;

    bool initializeX11();
    void cleanupX11();
    void processXEvent(XEvent& event);
};

#endif // X11_EVENT_RECORDER_H
