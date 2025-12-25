#include "service.h"
#include <QDebug>
#include <QString>
#include "CDBus.h"
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QThread>

DBusService::DBusService(QObject *parent) : QObject(parent)
{
    registerObject();

    bool res = QDBusConnection::systemBus().connect(        // 系统总线
                QString(),                                  // 服务名,为空则监听所有服务
                DBUS_PATH_AUTO_TEST,                        // 路径
                DBUS_INTERFACE_AUTO_TEST_SH,                // 接口
                "autoTestSetHardkeyRecordStatus",           // 信号
                this,                                       // 信号接收者
                SLOT(slotSetHardkeyRecordStatus(bool)));    // 槽函数(必须)

    if (!res)
    {
        qWarning() << "[DBusService] connectDbus failed" << ": autoTestSetHardkeyRecordStatus";
    }

    res = QDBusConnection::systemBus().connect(             // 系统总线
                QString(),                                  // 服务名,为空则监听所有服务
                DBUS_PATH_AUTO_TEST,                        // 路径
                DBUS_INTERFACE_AUTO_TEST_SH,                // 接口
                "autoTestHardkey",                          // 信号
                this,                                       // 信号接收者
                SLOT(onRecvHardkeyEvent(QString,QByteArray)));    // 槽函数(必须)

    if (!res)
    {
        qWarning() << "[DBusService] connectDbus failed" << ": autoTestHardkey";
    }

    res = QDBusConnection::systemBus().connect(             // 系统总线
                QString(),                                  // 服务名,为空则监听所有服务
                "/org/aili1/test",                          // 路径
                "org.aili1.test",                           // 接口
                "testParam",                                // 信号
                this,                                       // 信号接收者
                SLOT(slotRecvSingleParam(QDBusVariant)));    // 槽函数(必须)

    if (!res)
    {
        qWarning() << "[DBusService] connectDbus failed" << ": testParam";
    }
}

void DBusService::run()
{

}

void DBusService::registerObject()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    bool result = connection.registerService("org.aili1.test");
    if (!result)
    {
        qWarning() << "[DBusService] DBusService registerService failed";
    }
    result = connection.registerObject("/org/aili1/test",
                                       "org.aili1.test",
                                       this,
                                       static_cast<QDBusConnection::RegisterOptions>(QDBusConnection::ExportAllSlots)
                                       );
    if(!result)
    {
        qWarning() << "[DBusService] DBusService registerObject failed";
    }
}

QString DBusService::bytes2Str(const QByteArray &bytes)
{
    if(bytes.isEmpty())
    {
        return {};
    }
    QString result;
    for (char c : bytes)
    {
        quint8 byte = static_cast<quint8>(c);
        result += QString("0x%1 ").arg(byte, 2, 16, QLatin1Char('0')).toLower();
    }
    result = result.trimmed();
    return result;
}

void DBusService::slotSetHardkeyRecordStatus(bool bOpen)
{
    qDebug() << "[DBusService] func:" << __func__ << "param:" << bOpen;
}
void DBusService::onRecvHardkeyEvent(const QString &strEvent, const QByteArray &bytes)
{
    qDebug() << "[DBusService] func:" << __func__ << "param:" << strEvent << " "<< bytes2Str(bytes);
}
bool DBusService::slotRecvSingleParam(const QDBusVariant& param)
{
    QThread::sleep(3);
    qDebug() << "[DBusService] func:" << __func__ << "param:" << param.variant();
    return true;
}
