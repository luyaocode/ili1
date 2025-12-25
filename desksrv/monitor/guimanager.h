#ifndef GUIMANAGER_H
#define GUIMANAGER_H

#include <QObject>
#include <QList>
#include <QDate>
#include <QWidget>

class QCalendarWidget;
class QLabel;
class QVBoxLayout;

class GuiManager : public QObject
{
    Q_OBJECT
public:
    static GuiManager& getInst();
    void showCalendar(const QList<QDate> &highlightDates = QList<QDate>());
private slots:
    void onCalendarPageChanged(int year, int month);

private:
    explicit GuiManager(QObject *parent = nullptr);
    ~GuiManager();
    QWidget *createMainWindow(const QList<QDate> &highlightDates);
    // 将double类型时间（如8.5表示8.5小时）转换为"时:分"格式（如8:30）
    QString formatTime(double time);

private:
    QLabel      *m_statsLabel = nullptr;
    QWidget     *m_window     = nullptr;
    QList<QDate> m_markedDates;
};

#endif  // GUIMANAGER_H
