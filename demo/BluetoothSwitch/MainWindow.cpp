#include "MainWindow.h"
#include <QDebug>
#include <QThread>
#include <QMessageBox>
#include <QTime>
#include <QBluetoothDeviceInfo>
#include <QBluetoothLocalDevice>
#include <QDBusInterface>
#include "ui_MainWindow.h"
#include "TraceObject.h"
#include "MainThreadBlockDetector.h"

#define BLUEZ_DEVICE_INTERFACE "org.bluez.Device1"               // 蓝牙设备接口
#define BLUEZ_SERVICE "org.bluez"                                // 蓝牙服务
#define BLUEZ_PATH "/org/bluez"                                  // 蓝牙路径
#define BLUEZ_ADAPTER_INTERFACE "org.bluez.Adapter1"             // 蓝牙适配器接口
#define BLUEZ_AGENT_PATH "/org/qt/bluetooth/agent"               // 代理路径
#define BLUEZ_AGENT_MANAGER_INTERFACE "org.bluez.AgentManager1"  // 代理接口
#define MAX_TRANS_COUNT 3                                        // 最大传输次数
QString getMacFromDevicePath(const QString &devicePath)
{
    QDBusInterface devInterface(BLUEZ_SERVICE, devicePath, BLUEZ_DEVICE_INTERFACE, QDBusConnection::systemBus());
    if (devInterface.isValid())
    {
        // 从D-Bus对象中读取Address属性（即MAC地址，格式如 "AA:BB:CC:DD:EE:FF"）
        return devInterface.property("Address").toString();
    }
    return "";
}

QString getDevicePathFromMac(const QString &macAddr)
{
    // 标准化MAC地址：大写 + 替换冒号为下划线
    QString targetMac = macAddr.trimmed().toUpper().replace(':', '_');
    if (targetMac.isEmpty() || targetMac.count('_') != 5)
    {
        qWarning() << "Invalid MAC address format: " << macAddr;
        return "";
    }

    // 拼接devicePath（默认适配器hci0）
    QString devicePath = QString("/org/bluez/hci0/dev_%1").arg(targetMac);

    // 验证拼接的devicePath是否有效
    QDBusInterface devInterface(BLUEZ_SERVICE, devicePath, BLUEZ_DEVICE_INTERFACE, QDBusConnection::systemBus());
    if (devInterface.isValid())
    {
        return devicePath;
    }
    return "";
}

// elapsed < 1ms
bool isDevicePaired(const QBluetoothDeviceInfo &deviceInfo)
{
    QString        macAddr    = deviceInfo.address().toString().toUpper();
    QString        devicePath = getDevicePathFromMac(macAddr);
    QDBusInterface devInterface(BLUEZ_SERVICE, devicePath, BLUEZ_DEVICE_INTERFACE, QDBusConnection::systemBus());
    bool           isPaired = false;
    if (devInterface.isValid())
    {
        isPaired = devInterface.property("Paired").toBool();
    }
    return isPaired;
}
// elapsed < 1ms
bool isDeviceConnected(const QBluetoothDeviceInfo &deviceInfo)
{
    QString        macAddr    = deviceInfo.address().toString().toUpper();
    QString        devicePath = getDevicePathFromMac(macAddr);
    QDBusInterface devInterface(BLUEZ_SERVICE, devicePath, BLUEZ_DEVICE_INTERFACE, QDBusConnection::systemBus());
    bool           isConnected = false;
    if (devInterface.isValid())
    {
        isConnected = devInterface.property("Connected").toBool();
    }
    return isConnected;
}

const QString BTN_STYLE = R"(
       QPushButton {
           background-color: #E0E0E0; /* 未勾选时的默认灰色背景 */
           border: none;
           padding: 6px 12px;
           border-radius: 4px;
           color: #000000; /* 文字颜色 */
       }
       QPushButton:checked {
           background-color: #4CAF50; /* 勾选时的绿色背景（绿色2） */
           color: #FFFFFF; /* 勾选时文字改为白色，对比更明显 */
       }
       QPushButton:hover {
           opacity: 0.9; /* 鼠标悬浮时轻微透明，提升交互体验 */
       }
   )";

int        g_duration       = 0;                              // 耗时(ms)
int        g_interval_index = 0;                              // 测试项索引
QList<int> g_intervals      = {500, 1000, 1500, 2000, 2500};  // 测试项:开关频率(ms)

const int MAX_DURATION = 1000;  // 耗时阈值,超过阈值停止当前测试项
const int MAX_SWCOUNT  = 2000;  // 开关次数阈值,超过阈值停止当前测试项

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_pBlueMgr(new CBluetoothMgr(this))
{
    setupUi();
    initConn();
    m_timer.setInterval(g_intervals[g_interval_index]);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onActionPaintTriggered()
{
    m_pChartDlg = new ChartDialog(this);
    m_pChartDlg->show();
}

void MainWindow::setupUi()
{
    ui->setupUi(this);
    initBtn(ui->btnAgentSwitch);
    initBtn(ui->btnBlueSwitch);
    initBtn(ui->btnTimerSw);
    ui->tblScanElapsed->setColumnCount(4);
    ui->tblScanElapsed->setHorizontalHeaderLabels({"序号", "扫描耗时(ms)", "开始扫描耗时(ms)", "扫描时间"});
    ui->tblScanElapsed->horizontalHeader()->setStretchLastSection(true);

    ui->tblDiscoveredDevs->setColumnCount(4);
    ui->tblDiscoveredDevs->setHorizontalHeaderLabels({"序号", "设备", "是否配对", "是否连接"});
    ui->tblDiscoveredDevs->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::initBtn(QPushButton *btn)
{
    if (btn == nullptr)
    {
        return;
    }
    btn->setStyleSheet(BTN_STYLE);
    btn->setCheckable(true);
    btn->setChecked(false);
}

void MainWindow::initConn()
{
    //    connect(m_pBlueMgr->m_scanWorker, &CBluetoothScanWorker::sigPoweredOff, this, [this]() {
    //        QMessageBox::warning(this, "警告", "扫描设备需要先开启蓝牙!");
    //    });
    //    connect(m_pBlueMgr->m_scanWorker, &CBluetoothScanWorker::sigScanElapsed, this,
    //            [this](int scanElapsed, int startElapsed) {
    //                appendElapsedRow(scanElapsed, startElapsed);
    //            });
    connect(m_pBlueMgr->m_scanWorker, &CBluetoothScanWorker::sigDeviceDiscovered, this,
            [this](const QBluetoothDeviceInfo &info) {
                updateDevs(info);
            });
    connect(ui->btnBlueSwitch, &QPushButton::clicked, this, [this]() {
        m_pBlueMgr->ProSetBluetoothSwitch(ui->btnBlueSwitch->isChecked());
    });

    connect(ui->btnAgentSwitch, &QPushButton::clicked, this, [this]() {
        m_pBlueMgr->ProSetBluetoothAgentSwitch(ui->btnAgentSwitch->isChecked());
        QString info = QString("Current Interval(ms): %1, Click Count: %2, Duration(ms): %3")
                           .arg(g_intervals[g_interval_index])
                           .arg(m_clickCount)
                           .arg(g_duration);
        ui->lblClickCount->setText(info);
        m_clickCount++;
        if (m_clickCount >= MAX_SWCOUNT)
        {
            m_timer.stop();
            g_interval_index++;
            m_timer.stop();
            if (g_interval_index < g_intervals.size())
            {
                m_clickCount = 0;
                m_timer.setInterval(g_intervals[g_interval_index]);
                QTimer::singleShot(60 * 1000, [this]() {
                    m_timer.start();
                });
            }
            else
            {
                qDebug() << "Stop all.";
            }
        }
    });

    connect(&m_timer, &QTimer::timeout, this, [this]() {
        ui->btnAgentSwitch->click();
    });
    connect(ui->btnTimerSw, &QPushButton::clicked, this, [this]() {
        if (ui->btnTimerSw->isChecked())
        {
            if (!m_timer.isActive())
            {
                m_timer.start();
            }
        }
        else
        {
            if (m_timer.isActive())
            {
                m_timer.stop();
                m_clickCount = 0;
                ui->lblClickCount->setText(QString("Click Count: %1").arg(m_clickCount));
            }
        }
    });

    connect(ui->actionPaint, &QAction::triggered, this, &MainWindow::onActionPaintTriggered);
}

void MainWindow::appendElapsedRow(int scanElapsed, int startElapsed)
{
    int row = ui->tblScanElapsed->rowCount();
    ui->tblScanElapsed->insertRow(row);

    QTableWidgetItem *numItem = new QTableWidgetItem(QString::number(row + 1));
    numItem->setFlags(numItem->flags() & ~Qt::ItemIsEditable);
    ui->tblScanElapsed->setItem(row, 0, numItem);
    QTableWidgetItem *scanElapsedItem = new QTableWidgetItem(QString::number(scanElapsed));
    ui->tblScanElapsed->setItem(row, 1, scanElapsedItem);
    QTableWidgetItem *startElapsedItem = new QTableWidgetItem(QString::number(startElapsed));
    ui->tblScanElapsed->setItem(row, 2, startElapsedItem);
    QTime   endTime            = QTime::currentTime();
    QTime   startTime          = endTime.addMSecs(-scanElapsed);
    QString timeFormat         = "hh:mm:ss.zzz";
    QString timeRange          = QString("%1-%2").arg(startTime.toString(timeFormat)).arg(endTime.toString(timeFormat));
    QTableWidgetItem *timeItem = new QTableWidgetItem(timeRange);
    timeItem->setFlags(timeItem->flags() & ~Qt::ItemIsEditable);
    ui->tblScanElapsed->setItem(row, 3, timeItem);
}

void MainWindow::updateDevs(const QBluetoothDeviceInfo &info)
{
    bool bPaired    = isDevicePaired(info);
    bool bConnected = isDeviceConnected(info);
    // 1. 清空表格旧数据（避免重复）
    ui->tblDiscoveredDevs->setRowCount(0);

    // 2. 遍历 QSet 中的设备，逐行填充表格
    int index = 1;  // 序号从1开始
    // 2.1 添加新行
    ui->tblDiscoveredDevs->insertRow(index - 1);

    // 2.2 设置第一列：序号（转为字符串）
    QTableWidgetItem *indexItem = new QTableWidgetItem(QString::number(index));
    // 序号居中显示（可选）
    indexItem->setTextAlignment(Qt::AlignCenter);
    ui->tblDiscoveredDevs->setItem(index - 1, 0, indexItem);

    // 2.3 设置第二列：设备名称（处理空名称场景）
    QString devName = info.name();
    if (devName.isEmpty())
    {
        devName = "未知设备 (" + info.address().toString() + ")";
    }
    QTableWidgetItem *nameItem = new QTableWidgetItem(devName);
    ui->tblDiscoveredDevs->setItem(index - 1, 1, nameItem);

    QTableWidgetItem *pairItem = new QTableWidgetItem(bPaired ? "已配对" : "未配对");
    ui->tblDiscoveredDevs->setItem(index - 1, 2, pairItem);
    QTableWidgetItem *connectItem = new QTableWidgetItem(bConnected ? "已连接" : "未连接");
    ui->tblDiscoveredDevs->setItem(index - 1, 3, connectItem);
}

CBluetoothMgr::CBluetoothMgr(QObject *parent): QObject(parent), m_bSupportBluetooth(IsBluetoothSupport())
{
    if (!m_bSupportBluetooth)
    {
        return;
    }
    m_pDevice    = new QBluetoothLocalDevice(this);
    m_scanWorker = new CBluetoothScanWorker(this);
    connect(this, &CBluetoothMgr::sigSwitchScan, m_scanWorker, &CBluetoothScanWorker::slotSwitch, Qt::DirectConnection);
    m_scanWorker->startThread("td_bluetooth_scan_worker");
}

CBluetoothMgr::~CBluetoothMgr()
{
}

bool CBluetoothMgr::IsBluetoothSupport()
{
    bool bRet = false;
    //获取蓝牙设备
    //hciconfig | grep hci0
    QString strCommand = QString("hciconfig | grep hci0");

    auto pairRet = ProcessCommand(strCommand);
    //    qDebug() << "pairRet:" << pairRet;

    bRet = (!pairRet.second.isEmpty());
    return bRet;
}

QPair<bool, QString> CBluetoothMgr::ProcessCommand(const QString &strCmd)
{
    TRACEOBJECT(QString("ProcessCommand: %1").arg(strCmd).toStdString().c_str())
    m_ProcNormal.close();  // 关闭旧进程，避免冲突
    QPair<bool, QString> pairRet = {false, QString()};
    m_ProcNormal.setProgram("/bin/sh");
    m_ProcNormal.setArguments({"-c", strCmd});
    m_ProcNormal.start();
    m_ProcNormal.waitForStarted();
    m_ProcNormal.waitForFinished();
    QString strErr = m_ProcNormal.readAllStandardError();
    if (!strErr.isEmpty())
    {
        qWarning() << "strErr:" << strErr;
        return pairRet;
    }

    QString strOutPut = m_ProcNormal.readAllStandardOutput();
    //    qDebug()<<"ProcessCommand output:"<<strOutPut;
    pairRet = {true, strOutPut};
    return pairRet;
}

void CBluetoothMgr::ProSetBluetoothSwitch(bool bOn)
{
    if (!m_pDevice || !m_bSupportBluetooth)
    {
        return;
    }
    qDebug() << "ProSetBluetoothSwitch" << bOn;
    // 控制本地蓝牙适配器
    if (bOn)
    {
        ProcessCommand("bluetoothctl power on");
        ProcessCommand("bluetoothctl discoverable on");
        //设置自身可配对
        ProcessCommand("bluetoothctl pairable on");
    }
    else
    {
        ProcessCommand("bluetoothctl power off");
    }
}

void CBluetoothMgr::ProSetBluetoothAgentSwitch(bool bOn)
{
    if (!m_pDevice || !m_bSupportBluetooth)
    {
        return;
    }
    emit sigSwitchScan(bOn);
}
