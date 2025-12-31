#ifndef EYESPROTECTOR_H
#define EYESPROTECTOR_H
#include <QObject>
#include <QTimer>
#include "inputwatcher.h"

class EyesProtector : public QObject
{
    Q_OBJECT
public:
    explicit EyesProtector(InputWatcher *inputWatcher, QObject *parent = nullptr);
    bool start();
    bool stop();
private slots:
    void onUserActivity(const QString &event);

private:
    void setScheduledTask();

private:
    int           eyes_tolerance_minutes = 0;
    InputWatcher *m_inputWatcher         = nullptr;
    QTimer       *m_activityTimer        = nullptr;
    qint64        m_activitySeconds;            // 短时活动
    qint64        m_totalActivitySecondsToday;  // 全天活动时长
};

#endif  // EYEPROTECTOR_H
