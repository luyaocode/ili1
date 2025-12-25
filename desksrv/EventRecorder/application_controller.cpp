// application_controller.cpp
#include "application_controller.h"
#include "event_serializer.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QDateTime>
#include <QTimer>

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent), isPlaybackMode(false) {
    setupRecorderConnections();
    setupPlayerConnections();
}

void ApplicationController::setupRecorderConnections() {
    connect(&recorder, &X11EventRecorder::recordingStopped, this, &ApplicationController::onRecorderStopped);
    connect(&recorder, &X11EventRecorder::errorOccurred, this, &ApplicationController::onError);
}

void ApplicationController::setupPlayerConnections() {
    connect(&player, &X11EventPlayer::playbackFinished, this, &ApplicationController::onPlayerFinished);
    connect(&player, &X11EventPlayer::errorOccurred, this, &ApplicationController::onError);
}

void ApplicationController::initialize(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("X11GlobalEventRecorder");
    app.setApplicationVersion("2.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("Records and plays back global X11 mouse and keyboard events.");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption playbackOption(QStringList() << "b" << "playback",
                                      "Playback mode. Specify the recording file.", "filename");
    parser.addOption(playbackOption);

    parser.process(app);

    if (parser.isSet(playbackOption)) {
        isPlaybackMode = true;
        playbackFileName = parser.value(playbackOption);

        QList<RecordedEvent> events;
        if (EventSerializer::loadFromFile(playbackFileName, events)) {
            qInfo() << "Playback will start in 3 seconds. Switch to the target window now!";
            QTimer::singleShot(3000, [this, events]() {
                player.playEvents(events);
            });
        } else {
            emit finished(1);
            return;
        }
    } else {
        // 记录模式
        QString recordFileName = QString("recording_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
        qInfo() << "Recording mode. Press Ctrl+C to stop and save to" << recordFileName;

        // 注册一个信号处理器来优雅地停止记录
        // (在 Qt 控制台程序中处理 SIGINT 需要一些技巧，这里简化处理)
        // 实际应用中，可以使用 signal(SIGINT, handler) 并在 handler 中调用 recorder.stopRecording()
        // 为了演示，我们假设程序会在某个时刻被外部中断或调用 stopRecording
        recorder.startRecording();
    }

    emit finished(app.exec());
}

void ApplicationController::stopRecord()
{
    onRecorderStopped();
}

void ApplicationController::onRecorderStopped() {
    qInfo() << "Saving recorded events...";
    QString fileName = QString("recording_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    EventSerializer::saveToFile(fileName, recorder.getRecordedEvents());
    emit finished(0);
}

void ApplicationController::onPlayerFinished() {
    emit finished(0);
}

void ApplicationController::onError(const QString& errorMessage) {
    qCritical() << "Error:" << errorMessage;
    emit finished(1);
}
