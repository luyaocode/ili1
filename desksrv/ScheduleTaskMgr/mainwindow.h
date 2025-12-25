#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include "scheduleitem.h"
class ScheduleModel;
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(const QList<ScheduleItem>& schedules,QWidget *parent = nullptr);
    ~MainWindow();
    ScheduleModel *getScheduleModel() const;

private:
    void initUi();

private:
    QTableView    *m_scheduleView;
    ScheduleModel *m_scheduleModel;
};

#endif  // MAINWINDOW_H
