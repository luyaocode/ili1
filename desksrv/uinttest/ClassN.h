#ifndef CLASSN_H
#define CLASSN_H
#include <QObject>
#include <QReadWriteLock>
#include <QDebug>
class ClassN : public QObject
{
    Q_OBJECT
public:
    void tryLock()
    {
        // 1. 尝试获取读锁，必须判断返回值
        bool readOk = m_lock.tryLockForRead();
        if (readOk)
        {
            qDebug() << "Lock for read: success";
            // 读操作逻辑...

            // 2. 释放读锁后，再尝试写锁（禁止升级）
            //            m_lock.unlock();

            // 3. 尝试获取写锁，判断返回值
            bool writeOk = m_lock.tryLockForWrite();
            if (writeOk)
            {
                qDebug() << "Lock for write: success";
                // 写操作逻辑...
                m_lock.unlock();  // 释放写锁
            }
            else
            {
                m_lock.unlock();
                qDebug() << "Lock for write: failed";
            }
        }
        else
        {
            qDebug() << "Lock for read: failed";
        }
    }

private:
    QReadWriteLock m_lock;
};
#endif  // CLASSN_H
