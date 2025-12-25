#ifndef QUICKMANAGER_H
#define QUICKMANAGER_H
#include <QObject>
#include <QMap>
#include <QString>
#include "quickitem.h"
struct x11struct;
struct KeyWithModifier;
class KeyBlocker;
class MainWindow;
class ToolModel;
class QuickManager : public QObject

{
    Q_OBJECT
public:
    explicit QuickManager();
    ~QuickManager();
signals:
    void sigTriggerTool(const QString &toolpath);
private slots:
    void slotUpdateKeyBlockers(const QList<QuickItem> &list);
    void slotTriggerTool(const QString &toolpath);
    void slotBlocked();
    // 打开终端槽函数
    void slotOpenTerminal(const QString &path);
    // 打开目录槽函数
    void slotOpenDirectory(const QString &path);
    void onOpenResult(const QString &what, bool result);

private:
    void startKeyBlockers();
    bool loadConfig(const QString &fileName);

private:
    QScopedPointer<KeyBlocker>                m_AppBlocker;
    QMap<QString, QSharedPointer<KeyBlocker>> m_blockers;
    MainWindow                               *m_win = nullptr;
};
#endif  // QUICKMANAGER_H
