#include "client.h"
#include <QDebug>
#include <QString>
#include "CDBus.h"
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusPendingCallWatcher>
#include <QDBusReply>
#include <QDBusVariant>

DBusClient::DBusClient(QObject *parent) : QObject(parent)
{
}

void DBusClient::syncCall()
{
    QDBusInterface interface(
        "org.aili1.test",       // 服务名
        "/org/aili1/test",      // 对象路径
        "org.aili1.test",       // 接口名
        QDBusConnection::sessionBus()  // 会话总线
    );

    // 检查接口是否有效
    if (!interface.isValid()) {
        qDebug() << "接口无效：" << QDBusConnection::sessionBus().lastError().message();
        return;
    }
    QDBusVariant dbusVar(12);
    QVariant param = QVariant::fromValue(dbusVar);  // 显式转换为QVariant
    QDBusMessage reply = interface.call("slotRecvSingleParam", param);

    // 处理结果：判断是否为有效回复
    if (reply.type() == QDBusMessage::ReplyMessage)
    {
        if (!reply.arguments().isEmpty())
        {
            double result = reply.arguments().first().toBool();
            qDebug() << "同步调用返回结果：" << result;
        } else
        {
            qDebug() << "同步调用成功，但无返回值";
        }
    } else
    {
        qDebug() << "同步调用失败：" << reply.errorMessage();
    }

}

void DBusClient::asyncCall()
{
    // 异步调用
    QDBusInterface interface(
        "org.aili1.test",
        "/org/aili1/test",
        "org.aili1.test",
        QDBusConnection::sessionBus()
    );

    if (!interface.isValid()) {
        qDebug() << "接口无效：" << QDBusConnection::sessionBus().lastError().message();
        return;
    }

    // 发起异步调用并绑定回调
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(
        interface.asyncCall("slotRecvSingleParam", 1.234),  // 异步调用
        nullptr
    );

    // 回调处理
    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
        [watcher](QDBusPendingCallWatcher* self) {
            QDBusReply<bool> reply = *self;  // 解析返回值（这里已知为bool）
            if (reply.isValid())
            {
                qDebug() << "异步结果：" << reply.value();
            } else
            {
                qDebug() << "调用失败：" << reply.error().message();
            }
            self->deleteLater();  // 释放资源
        }
    );
}

void DBusClient::signal()
{
    QDBusMessage msg = QDBusMessage::createSignal(DBUS_PATH_AUTO_TEST, DBUS_INTERFACE_AUTO_TEST_SH, "autoTestSetHardkeyRecordStatus");
    msg.setArguments(QVariantList() << false);

    bool res = QDBusConnection::systemBus().send(msg);

    if (!res)
    {
        qWarning() << "[client] sendDbusMessage failed";
    }
}

void DBusClient::registerObject()
{

}
