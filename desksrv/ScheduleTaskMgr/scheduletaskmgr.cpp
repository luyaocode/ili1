#include "scheduletaskmgr.h"
#include <QTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include "mainwindow.h"
#include "commontool/commontool.h"
#include "tool.h"
#include "unify/asynctaskmanager.hpp"
#include "commontool/multischeduledtaskmgr.h"
#include "def.h"
#include "scheduleitem.h"
#include "schedulemodel.h"

#include "commontool/globaltool.h"
#include "commontool/x11struct.h"
#include "commontool/keyblocker.h"

USING_NAMESAPCE(unify)
ScheduleTaskMgr::ScheduleTaskMgr()
{
    startKeyBlocker();
    loadConfig();
}

ScheduleTaskMgr::~ScheduleTaskMgr()
{
    if (m_win)
    {
        delete m_win;
        m_win = nullptr;
    }
}

void ScheduleTaskMgr::slotBlocked()
{
    if (m_win)
    {
        m_win->setWindowFlags(m_win->windowFlags() | Qt::WindowStaysOnTopHint);
        m_win->show();            // 重新显示以应用标志
        m_win->raise();           // 提升层级
        m_win->activateWindow();  // 激活窗口

        // 3. 取消置顶（避免长期占用最前）
        QTimer::singleShot(50, [this]() {
            if (m_win && m_win->isVisible())
            {
                m_win->setWindowFlags(m_win->windowFlags() & ~Qt::WindowStaysOnTopHint);
                m_win->show();  // 应用取消后的标志
            }
        });
        return;
    }
    // 创建新窗口
    m_win = new MainWindow(m_schedules);
    m_win->setAttribute(Qt::WA_DeleteOnClose);  // 强制关闭时析构窗口

    // 使用 Qt::UniqueConnection 避免重复连接
    connect(m_win->getScheduleModel(), &ScheduleModel::sigSaveConfig, this, &ScheduleTaskMgr::slotSaveConfig,
            Qt::UniqueConnection);
    connect(m_win, &MainWindow::destroyed, this, [this]() {
        m_win = nullptr;  // 关键
    });

    m_win->show();
    m_win->raise();           // 提升层级
    m_win->activateWindow();  // 激活窗口
}

void ScheduleTaskMgr::slotSaveConfig(const QList<ScheduleItem>& items)
{
    // 更新计划
    updateScheduleTasks(items);
    // 更新配置文件
    updateConfig();
}

void ScheduleTaskMgr::startKeyBlocker()
{
    // 初始化全局快捷键（Ctrl+Shift+S）
    m_AppBlocker.reset(new KeyBlocker({KeyWithModifier(XK_T, ControlMask | ShiftMask)}));
    connect(m_AppBlocker.data(), &KeyBlocker::sigBlocked, this, &ScheduleTaskMgr::slotBlocked);
    m_AppBlocker->start();
}

void ScheduleTaskMgr::loadConfig()
{
    QString realFilePath = Commontool::getConfigFilePath(APP_NAME, CONFIG_FILE);
    QFile   file(realFilePath);

    if (!file.exists())
    {
        qInfo() << "Config file not found: " << realFilePath;
        createConfigFile(realFilePath);
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open config file:" << realFilePath;
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument   doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return;
    }

    if (!doc.isArray())
    {
        qWarning() << "Invalid config file format (not array)";
        return;
    }
    QJsonArray array = doc.array();
    for (const QJsonValue &val : array)
    {
        if (val.isObject())
        {
            ScheduleItem item        = ScheduleItem::fromJson(val.toObject());
            QString      path        = item.path;
            item.taskConfig.callback = [path]() {
                if (path.isEmpty())
                {
                    return;
                }
                QStringList list = path.split(' ');
                QString     cmd  = list[0];
                QStringList params;
                if (list.size() > 1)
                {
                    params = list.mid(1);
                }
                Commontool::executeThirdProcess(cmd, params);
            };
            m_schedules.append(item);
        }
    }
    startScheduleTasks();
}

void ScheduleTaskMgr::startScheduleTasks()
{
    for (auto &item : m_schedules)
    {
        if (!item.enable)
        {
            continue;
        }
        MultiScheduledTaskMgr::getInstance().addTask(item.taskConfig);
    }
}

void ScheduleTaskMgr::updateScheduleTasks(const QList<ScheduleItem>& items)
{
    for (const ScheduleItem &item : items)
    {
        MultiScheduledTaskMgr::getInstance().updateTask(item.taskConfig);
    }
}

void ScheduleTaskMgr::updateConfig()
{
    const QList<ScheduleItem> data = m_win->getScheduleModel()->getData();

    QJsonArray array;
    for (const ScheduleItem &item : data)
    {
        QJsonObject obj = item.toJson();
        array.append(obj);
    }
    QString realFilePath = Commontool::getConfigFilePath(APP_NAME, CONFIG_FILE);
    QFile   file(realFilePath);

    if (!file.exists())
    {
        qInfo() << "Config file not found, creating new one:" << realFilePath;
        createConfigFile(realFilePath);
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Failed to save config file:" << CONFIG_FILE;
        return;
    }

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
}
