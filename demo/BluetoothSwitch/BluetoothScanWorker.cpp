#include "BluetoothScanWorker.h"
#include <QTimer>
#include <QThread>
#include <QApplication>
#include <QDebug>

CBluetoothScanWorker::CBluetoothScanWorker(QObject *parent)
    : m_pAgent(new QBluetoothDeviceDiscoveryAgent(this)), m_pThread(new QThread(parent))
{
    m_pAgent->setLowEnergyDiscoveryTimeout(0);
    initConnect();
}

CBluetoothScanWorker::~CBluetoothScanWorker()
{
    slotStopWork();
}

void CBluetoothScanWorker::startThread(const QString &threadName)
{
    if (!m_pThread || m_pThread->isRunning())
    {
        return;
    }
    qDebug() << "CBluetoothScanWorker startThread";
    m_pThread->setObjectName(threadName);
    this->moveToThread(m_pThread);
    QMetaObject::invokeMethod(this, "slotStartWork", Qt::QueuedConnection);
    m_pThread->start();
}

void CBluetoothScanWorker::slotSwitch(bool on)
{
    qDebug() << "CBluetoothScanWorker slotSwitch" << on;
    if (on)
    {
        slotScanTimeout();
    }
    else
    {
        stopScanTimer();  // 停止扫描定时器
    }
}

void CBluetoothScanWorker::slotStartWork()
{
    m_pScanTimer = new QTimer(this);
    m_pScanTimer->setInterval(20000);
    m_pScanTimer->setSingleShot(true);  // 采用单次计时器防止事件堆积
    // 每隔20秒重启一次扫描
    connect(m_pScanTimer, &QTimer::timeout, this, &CBluetoothScanWorker::slotScanTimeout);
    m_pScanTimer->start();
}

void CBluetoothScanWorker::slotStopWork()
{
    if (m_pScanTimer && m_pScanTimer->isActive())
    {
        m_pScanTimer->stop();
    }
    if (m_pAgent && m_pAgent->isActive())
    {
        m_pAgent->disconnect();
        m_pAgent->stop();
    }
    if (m_pThread && m_pThread->isRunning())
    {
        m_pThread->quit();
        if (!m_pThread->wait(3000))
        {
            m_pThread->terminate();
            m_pThread->wait();
        }
    }
}

void CBluetoothScanWorker::slotScanFinished()
{
    qDebug() << "QBluetoothDeviceDiscoveryAgent finished";
    emit sigScanFinished();
}

void CBluetoothScanWorker::slotScanCanceled()
{
    qDebug() << "QBluetoothDeviceDiscoveryAgent canceled";
    emit sigScanFinished();
}

void CBluetoothScanWorker::slotScanTimeout()
{
    startScan();
    wakeupScanTimer();
}

void CBluetoothScanWorker::initConnect()
{
    if (!m_pAgent)
    {
        return;
    }
    connect(m_pAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this,
            &CBluetoothScanWorker::sigDeviceDiscovered, Qt::DirectConnection);
    connect(m_pAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &CBluetoothScanWorker::slotScanFinished,
            Qt::DirectConnection);
    connect(m_pAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &CBluetoothScanWorker::slotScanCanceled,
            Qt::DirectConnection);
    connect(
        m_pAgent,
        static_cast<void (QBluetoothDeviceDiscoveryAgent::*)(QBluetoothDeviceDiscoveryAgent::Error)>(
            &QBluetoothDeviceDiscoveryAgent::error),
        this,
        [this](QBluetoothDeviceDiscoveryAgent::Error error) {
            qDebug() << "QBluetoothDeviceDiscoveryAgent::Error code: " << error;
            emit sigScanError(error);
            stopScanTimer();
        },
        Qt::DirectConnection);

    QObject::connect(qApp, &QApplication::aboutToQuit, this, &CBluetoothScanWorker::slotStopWork, Qt::QueuedConnection);
}

void CBluetoothScanWorker::startScan()
{
    if (!m_pAgent)
    {
        return;
    }
    if (m_pAgent->isActive())
    {
        qDebug() << "QBluetoothDeviceDiscoveryAgent stop agent";
        m_pAgent->stop();
    }
    qDebug() << "CBluetoothScanWorker startScan";
    m_elapsedTimer.start();
    m_pAgent->start(QBluetoothDeviceDiscoveryAgent::ClassicMethod | QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
    qDebug() << "agent start elapsed: " << m_elapsedTimer.elapsed() << "ms.";
}

void CBluetoothScanWorker::stopScan()
{
    if (!m_pAgent)
    {
        return;
    }
    if (m_pAgent->isActive())
    {
        qDebug() << "CBluetoothScanWorker stopScan";
        m_pAgent->stop();
    }
}

// 扫描定时器
void CBluetoothScanWorker::wakeupScanTimer()
{
    if (m_pScanTimer && !m_pScanTimer->isActive())
    {
        qDebug() << "wakeupScanTimer";
        m_pScanTimer->start();
    }
}

void CBluetoothScanWorker::stopScanTimer()
{
    if (m_pScanTimer && m_pScanTimer->isActive())
    {
        qDebug() << "stopScanTimer";
        m_pScanTimer->stop();
    }
}
