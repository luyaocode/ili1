// application_controller.h
#ifndef APPLICATION_CONTROLLER_H
#define APPLICATION_CONTROLLER_H

#include <QObject>
#include <QString>
#include "x11_event_recorder.h"
#include "x11_event_player.h"

class ApplicationController : public QObject {
    Q_OBJECT
public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() = default;

    void initialize(int argc, char *argv[]);
    void stopRecord();
signals:
    void finished(int exitCode = 0);

private slots:
    void onRecorderStopped();
    void onPlayerFinished();
    void onError(const QString& errorMessage);

private:
    X11EventRecorder recorder;
    X11EventPlayer player;
    QString playbackFileName;
    bool isPlaybackMode;

    void setupRecorderConnections();
    void setupPlayerConnections();
};

#endif // APPLICATION_CONTROLLER_H
