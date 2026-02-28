/********************************************************************
 * Copyright (c) 2022 Comen Instruments,Inc.
 * All rights reserved.
 *
 * Project:    ComenUltrasound
 * Date:       2026-01-23
 * Author:     chenluyao
 * Brief:      蓝牙扫描线程工作类
 *********************************************************************/
#ifndef CBLUETOOTHSCANWORKER_H
#define CBLUETOOTHSCANWORKER_H

#include <QObject>
#include <QMutex>
#include <QBluetoothDeviceInfo>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QElapsedTimer>
class QThread;
class QTimer;

// 蓝牙扫描线程工作类
class CBluetoothScanWorker : public QObject
{
    Q_OBJECT
public:
    explicit CBluetoothScanWorker(QObject *parent = nullptr);
    ~CBluetoothScanWorker();
    // 开启线程
    void startThread(const QString &threadName);
public slots:
    // 扫描开关
    void slotSwitch(bool on);
signals:
    void sigDeviceDiscovered(const QBluetoothDeviceInfo &info);      // 设备发现
    void sigScanFinished();                                          // 扫描结束
    void sigScanError(QBluetoothDeviceDiscoveryAgent::Error error);  // 错误
    void sigScanCanceled();                                          // 取消扫描
private slots:
    // 开始工作
    void slotStartWork();
    // 终止工作
    void slotStopWork();
    // 扫描完成
    void slotScanFinished();
    // 扫描取消
    void slotScanCanceled();
    // 扫描定时器槽函数
    void slotScanTimeout();

private:
    // 连接信号槽
    void initConnect();
    // 开始扫描
    void startScan();
    // 停止扫描
    void stopScan();
    // 唤醒扫描定时器
    void wakeupScanTimer();
    // 停止扫描定时器
    void stopScanTimer();

private:
    QBluetoothDeviceDiscoveryAgent *m_pAgent     = nullptr;  // 扫描代理
    QThread                        *m_pThread    = nullptr;  // 线程
    QTimer                         *m_pScanTimer = nullptr;  // 扫描定时器
    QElapsedTimer                   m_elapsedTimer;
};

#endif  // CBLUETOOTHSCANWORKER_H
