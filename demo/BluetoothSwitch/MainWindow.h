#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QTimer>
#include <QProcess>
#include "BluetoothScanWorker.h"
#include "ChartDialog.h"
namespace Ui
{
    class MainWindow;
}

class CBluetoothMgr : public QObject
{
    Q_OBJECT
public:
    explicit CBluetoothMgr(QObject *parent);
    ~CBluetoothMgr();
    bool                 IsBluetoothSupport();
    QPair<bool, QString> ProcessCommand(const QString &strCmd);
    void                 ProSetBluetoothSwitch(bool bOn);
    void                 ProSetBluetoothAgentSwitch(bool bOn);
signals:
    void sigSwitchScan(bool on);

public:
    QBluetoothLocalDevice *m_pDevice    = nullptr;  // 注意,不支持蓝牙的话为空
    CBluetoothScanWorker   *m_scanWorker = nullptr;  // 注意,不支持蓝牙的话为空
    QProcess               m_ProcNormal;
    bool                   m_bSupportBluetooth = false;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void onActionPaintTriggered();

private:
    void setupUi();
    void initBtn(QPushButton *btn);
    void initConn();
    void appendElapsedRow(int scanElapsed, int startElapsed);
    void updateDevs(const QBluetoothDeviceInfo& info);

private:
    Ui::MainWindow *ui          = nullptr;
    ChartDialog    *m_pChartDlg = nullptr;
    CBluetoothMgr  *m_pBlueMgr  = nullptr;
    QTimer          m_timer;
    int             m_clickCount = 0;
};

#endif  // MAINWINDOW_H
