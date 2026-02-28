#ifndef DOUBLEFREETEST_H
#define DOUBLEFREETEST_H
#include <QCoreApplication>
#include <QList>
#include <QDebug>
#include <QTimer>

class Parent : public QObject
{
    Q_OBJECT
public:
    Parent(QObject *parent = nullptr): QObject(parent)
    {
    }
private:
    int id;
};

class MockProp : public QObject
{
    Q_OBJECT
public:
    MockProp(QObject *parent = nullptr): QObject(parent)
    {
        qDebug() << "MockProp创建:" << (void *)this;
    }
    ~MockProp()
    {
        qDebug() << "MockProp销毁:" << (void *)this;
    }
};

extern QList<MockProp *> propList;
extern QCoreApplication *g_app;

// 模拟DBus的InterfacesAdded回调（单线程，通过事件循环触发）
void mockDBusCallback(Parent *parent);

void reproduceNonAtomic(Parent *parent);

#endif  // DOUBLEFREETEST_H
