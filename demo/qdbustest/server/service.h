#ifndef SERVICE_H
#define SERVICE_H

#include <QObject>
#include <QDBusVariant>

// 服务端核心类：注册 D-Bus 接口、提供方法和信号
class DBusService : public QObject
{
    Q_OBJECT
    // 如果通过registerObject注册该类的话,该类自动暴露接口名称
    // 否则,无影响
    Q_CLASSINFO("D-Bus Interface", "org.ili1.test")

public:
    explicit DBusService(QObject *parent = nullptr);
    void run();
public slots:
    // 参数测试
    bool slotRecvSingleParam(const QDBusVariant& param);
private slots:
    void slotSetHardkeyRecordStatus(bool bOpen);
    void onRecvHardkeyEvent(const QString& strEvent,const QByteArray& bytes = QByteArray());
private:
    void registerObject();
    QString bytes2Str(const QByteArray &bytes);
};

#endif // SERVICE_H
