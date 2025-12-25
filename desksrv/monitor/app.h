#ifndef APP_H
#define APP_H
#include "monitor.h"
#include "extprocess.h"
#include "inputwatcher.h"
#include "eyesprotector.h"

class App : public QObject
{
    Q_OBJECT
public:
    explicit App();
    ExtProcess &getReminder();
    bool      run();

private:
    InputWatcher   m_inputWatcher;
    ExtProcess       m_reminder;
    Monitor       *m_monitor       = nullptr;
    EyesProtector *m_eyesProtector = nullptr;
};
#endif  // APP_H
