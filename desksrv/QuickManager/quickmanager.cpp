#include "quickmanager.h"
#include <QTimer>
#include <QMessageBox>
#include <QCoreApplication>
#include <QPointer>
#include "mainwindow.h"
#include "tool.h"
#include "commontool/x11struct.h"
#include "commontool/keyblocker.h"
#include "commontool/commontool.h"
#include "unify/asynctaskmanager.hpp"

USING_NAMESAPCE(unify)

QuickManager::QuickManager()
{
    startKeyBlockers();
    loadConfig("config.json");
    connect(this, &QuickManager::sigTriggerTool, this, &QuickManager::slotTriggerTool);
}

QuickManager::~QuickManager()
{
    if (m_win)
    {
        delete m_win;
        m_win = nullptr;
    }
}

void QuickManager::slotUpdateKeyBlockers(const QList<QuickItem> &list)
{
    if (list.isEmpty())
    {
        return;
    }
    for (const auto &item : list)
    {
        KeyWithModifier keymod;
        getKeyAndModifer(item, keymod);
        if (keymod.keysym == 0)
        {
            continue;
        }
        auto toolpath = item.path;
        auto callback = [this, toolpath]() {
            emit sigTriggerTool(toolpath);
        };
        keymod.callback = std::move(callback);
        m_blockers[item.name].reset(new KeyBlocker({keymod}));
        m_blockers[item.name]->start();
    }
}

void QuickManager::startKeyBlockers()
{
    // 初始化全局快捷键（Ctrl+Shift+S）
    m_AppBlocker.reset(new KeyBlocker({KeyWithModifier(XK_S, ControlMask | ShiftMask)}));
    connect(m_AppBlocker.data(), &KeyBlocker::sigBlocked, this, &QuickManager::slotBlocked);
    m_AppBlocker->start();
}

bool QuickManager::loadConfig(const QString &fileName)
{
    // 获取真实的配置文件路径（Linux 自动转换为 ~/.config/QuickManager/config.json）
    QString realFilePath = Commontool::getConfigFilePath("QuickManager", fileName);
    QFile   file(realFilePath);

    // 若文件不存在，创建空配置文件（避免首次运行解析失败）
    if (!file.exists())
    {
        qInfo() << "Config file not found, creating new one:" << realFilePath;
        createConfigFile(realFilePath);
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open config file:" << realFilePath;
        return false;
    }

    // Qt5.9 直接使用 QByteArray 解析 JSON
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument   doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return false;
    }

    if (!doc.isArray())
    {
        qWarning() << "Invalid config file format (not array)";
        return false;
    }

    QList<QuickItem> items;
    QJsonArray       array = doc.array();
    for (const QJsonValue &val : array)
    {
        if (val.isObject())
        {
            items.append(QuickItem::fromVariantMap(val.toObject().toVariantMap()));
        }
    }
    if (items.isEmpty())
    {
        return false;
    }
    for (const auto &item : items)
    {
        KeyWithModifier keymod;
        bool            succ = getKeyAndModifer(item, keymod);
        if (!succ)
        {
            continue;
        }
        auto toolpath   = item.path;
        keymod.callback = [this, toolpath]() {
            emit sigTriggerTool(toolpath);
        };
        m_blockers[item.name].reset(new KeyBlocker({keymod}));
        m_blockers[item.name]->start();
    }
    return true;
}

void QuickManager::slotTriggerTool(const QString &toolpath)
{
    if (toolpath.isEmpty())
    {
        return;
    }
    QStringList list = toolpath.split(' ');
    QString     cmd  = list[0];
    QStringList params;
    if (list.size() > 1)
    {
        params = list.mid(1);
    }
    Commontool::executeThirdProcess(cmd, params);
}

void QuickManager::slotBlocked()
{
    if (m_win)
    {
        m_win->setWindowFlags(m_win->windowFlags() | Qt::WindowStaysOnTopHint);
        m_win->show();            // 重新显示以应用标志（Qt 5.9 必须调用）
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
    m_win = new MainWindow;
    m_win->setAttribute(Qt::WA_DeleteOnClose);  // 强制关闭时析构窗口

    // 使用 Qt::UniqueConnection 避免重复连接
    connect(m_win->getToolModel(), &ToolModel::sigUpdateKeyBlockers, this, &QuickManager::slotUpdateKeyBlockers,
            Qt::UniqueConnection);
    connect(m_win->getCommonModel(), &CommonModel::sigUpdateKeyBlockers, this, &QuickManager::slotUpdateKeyBlockers,
            Qt::UniqueConnection);
    connect(m_win, &MainWindow::destroyed, this, [this]() {
        m_win = nullptr;  // 关键
    });
    connect(m_win, &MainWindow::sigOpenTerminal, this, &QuickManager::slotOpenTerminal, Qt::UniqueConnection);
    connect(m_win, &MainWindow::sigOpenDirectory, this, &QuickManager::slotOpenDirectory, Qt::UniqueConnection);

    m_win->show();
    m_win->raise();           // 提升层级
    m_win->activateWindow();  // 激活窗口
}

void QuickManager::slotOpenTerminal(const QString &path)
{
    auto task_cb = [path](const AsyncTaskRunner<bool, float>::UpdateCallback &,
                          const AsyncTaskRunner<bool, float>::IsCanceledCallback &) {
        return Commontool::openTerminal(path);
    };
    QPointer<MainWindow> winp        = QPointer<MainWindow>(m_win);
    auto                 complete_cb = [this, winp](unify::EM_TASK_STATE, bool result) {
        QMetaObject::invokeMethod(this, "onOpenResult", Q_ARG(QString, "终端"), Q_ARG(bool, result));
    };
    AsyncTaskManager::get_instance().add_task<bool>(task_cb, nullptr, nullptr, complete_cb);
}

void QuickManager::slotOpenDirectory(const QString &path)
{
    auto task_cb = [path](const AsyncTaskRunner<bool, float>::UpdateCallback &,
                          const AsyncTaskRunner<bool, float>::IsCanceledCallback &) {
        return Commontool::openLocalUrl(path);
    };
    QPointer<MainWindow> winp        = QPointer<MainWindow>(m_win);
    auto                 complete_cb = [this, winp](unify::EM_TASK_STATE, bool result) {
        QMetaObject::invokeMethod(this, "onOpenResult", Q_ARG(QString, "目录"), Q_ARG(bool, result));
    };
    AsyncTaskManager::get_instance().add_task<bool>(task_cb, nullptr, nullptr, complete_cb);
}

void QuickManager::onOpenResult(const QString &what, bool result)
{
    if (!result)
    {
        QMessageBox::information(m_win, "打开" + what, result ? "打开" + what + "成功" : "打开" + what + "失败");
    }
}
