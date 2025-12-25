#ifndef SCHEDULETASKMGR_H
#define SCHEDULETASKMGR_H

#include <QObject>
#include <QMutex>
#include "commontool/multischeduledtaskmgr.h"
class MainWindow;
class KeyBlocker;
struct ScheduleItem;
struct KeyWithModifier;
class ScheduleTaskMgr : public QObject

{
    Q_OBJECT
public:
    explicit ScheduleTaskMgr();
    ~ScheduleTaskMgr();
signals:
private slots:
    void slotBlocked();
    void slotSaveConfig(const QList<ScheduleItem>& items);

private:
    void startKeyBlocker();
    void loadConfig();
    void startScheduleTasks();
    void updateScheduleTasks(const QList<ScheduleItem>& items);
    void updateConfig();

private:
    QScopedPointer<KeyBlocker> m_AppBlocker;
    MainWindow                *m_win = nullptr;
    QList<ScheduleItem>        m_schedules;
};

#endif  // SCHEDULETASKMGR_H
