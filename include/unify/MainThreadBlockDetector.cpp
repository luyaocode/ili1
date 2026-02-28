#include <QObject>
#include <QTimer>
#include <QElapsedTimer>
#include <QDebug>

class MainThreadBlockDetector : public QObject
{
    Q_OBJECT
public:
    explicit MainThreadBlockDetector(int      checkInterval  = 50,  // 检测间隔（ms）
                                     int      blockThreshold = 10,  // 阻塞阈值（ms）
                                     QObject *parent         = nullptr)
        : QObject(parent), m_checkInterval(checkInterval), m_blockThreshold(blockThreshold)
    {
        m_timer = new QTimer(this);
        m_timer->setInterval(m_checkInterval);
        m_timer->setTimerType(Qt::PreciseTimer);  // 高精度定时器

        m_elapsedTimer = new QElapsedTimer();

        connect(m_timer, &QTimer::timeout, this, [this]() {
            // 首次触发：启动计时器
            if (!m_elapsedTimer->isValid())
            {
                m_elapsedTimer->start();
                return;
            }

            // 计算实际触发间隔（正常应≈checkInterval）
            qint64 actualInterval = m_elapsedTimer->elapsed();
            m_elapsedTimer->restart();

            // 计算阻塞时长（实际间隔 - 预期间隔）
            qint64 blockTime = actualInterval - m_checkInterval;

            if (blockTime > m_blockThreshold)
            {
                qCritical() << "主线程阻塞！阻塞时长：" << blockTime << "ms（实际间隔：" << actualInterval << "ms）";
                // 可选：输出调用栈，定位阻塞代码（需要Qt 5.14+）
                // qDebug() << "阻塞时调用栈：" << QThread::currentThread()->stackTrace();
            }
            else
            {
                // 调试用：输出正常间隔
                // qDebug() << "主线程正常，间隔：" << actualInterval << "ms";
            }
        });

        // 启动检测器
        m_timer->start();
    }

private:
    QTimer        *m_timer        = nullptr;
    QElapsedTimer *m_elapsedTimer = nullptr;
    int            m_checkInterval;   // 预期检测间隔
    int            m_blockThreshold;  // 阻塞判定阈值
};
