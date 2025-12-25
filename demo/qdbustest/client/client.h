#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>

class DBusClient : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.example.client")

public:
    explicit DBusClient(QObject *parent = nullptr);
    void signal();      // 信号
    void syncCall();    // 同步调用
    void asyncCall();   // 异步调用
private:
    void registerObject();
};

#endif // CLIENT_H
