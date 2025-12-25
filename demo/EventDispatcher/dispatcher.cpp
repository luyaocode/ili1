#include "dispatcher.h"
#include <QDebug>
#include <QElapsedTimer>


DispatcherThread::DispatcherThread(QObject *parent)
    : QThread(parent)
{
    m_pool.setMaxThreadCount(3);
    m_pool.setExpiryTimeout(2000);
    qDebug() << "[事件分发器] 线程池初始化完成，最大线程数：" << m_pool.maxThreadCount();
}

DispatcherThread::~DispatcherThread()
{
    m_pool.waitForDone();
    quit();
    wait();
}

void DispatcherThread::sendDbusMessage(const char *interfaceName, const QString &strName, const QVariantList &args, const QString &strPath)
{
    // TODO
}

void DispatcherThread::run()
{
    qDebug() << "[DispatcherThread]" << QThread::currentThreadId() << "启动事件循环";
    exec();
}
