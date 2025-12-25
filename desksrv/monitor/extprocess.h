#ifndef REMINDER_H
#define REMINDER_H
#include <QObject>
#include <QMutex>

class ExtProcess : public QObject
{
    Q_OBJECT
public:
    ExtProcess();
    // 屏幕保护弹窗
    static void eyesProtect();
    // 简单消息弹窗
    static void simplePopup(const QString &msg);
    // 录制屏幕
    static void recordScreen(int seconds, const QString &filePath = "");
    // 截屏
    static void screenShoot();

private:
    static void process(const QString &cmd, const QStringList &params = {});
    void        checkReminderTime();

private:
    QMutex mutex;
};
#endif  // REMINDER_H
