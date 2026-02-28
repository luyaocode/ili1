#ifndef PCIEDMAWIDGET_H
#define PCIEDMAWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QFile>
#include <QThread>
#include <QMutex>
#include <QMessageBox>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

/**
 * @brief The PCIeDMAWidget class
 * 核心功能：Qt界面化的PCIe DMA读写工具
 * 适配场景：Linux下PCIe/FPGA设备的DMA交互，模拟之前遇到的「WriteDma」「设备忙」问题
 * 核心特性：
 * 1. 图形化操作PCIe DMA设备（读写寄存器、DMA数据传输）
 * 2. 实时显示操作状态和错误信息
 * 3. 模拟「设备忙」「权限不足」「地址错误」等异常场景
 * 4. 线程安全的DMA操作（避免UI阻塞）
 */
class PCIeDMAWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief PCIeDMAWidget 构造函数
     * @param parent 父窗口指针
     */
    explicit PCIeDMAWidget(QWidget *parent = nullptr);
    ~PCIeDMAWidget() override;

    /**
     * @brief DMA操作状态枚举
     */
    enum DMAStatus {
        DMA_Idle,        // 空闲
        DMA_Writing,     // 正在写
        DMA_Reading,     // 正在读
        DMA_Error,       // 错误
        DMA_DeviceBusy   // 设备忙
    };
    Q_ENUM(DMAStatus)

private slots:
    /**
     * @brief onBtnOpenDeviceClicked 打开PCIe设备按钮点击槽函数
     */
    void onBtnOpenDeviceClicked();

    /**
     * @brief onBtnCloseDeviceClicked 关闭PCIe设备按钮点击槽函数
     */
    void onBtnCloseDeviceClicked();

    /**
     * @brief onBtnDMAWriteClicked DMA写操作按钮点击槽函数
     */
    void onBtnDMAWriteClicked();

    /**
     * @brief onBtnDMAReadClicked DMA读操作按钮点击槽函数
     */
    void onBtnDMAReadClicked();

    /**
     * @brief updateDMAStatus 更新DMA状态显示
     * @param status 新状态
     * @param msg 状态描述信息
     */
    void updateDMAStatus(DMAStatus status, const QString& msg);

    /**
     * @brief handleDMAError 处理DMA操作错误
     * @param errMsg 错误信息
     */
    void handleDMAError(const QString& errMsg);

private:
    /**
     * @brief initUI 初始化UI界面
     */
    void initUI();

    /**
     * @brief initConnections 初始化信号槽连接
     */
    void initConnections();

    /**
     * @brief writeDMAData DMA写操作核心函数（模拟CPcieDevice::WriteDma）
     * @param devPath PCIe设备路径（如/dev/pcie_dma）
     * @param data 要写入的数据
     * @param length 数据长度
     * @return 成功返回true，失败返回false
     * @note 模拟「Device or resource busy」「权限不足」等异常
     */
    bool writeDMAData(const QByteArray& data, qint64 length);

    /**
     * @brief readDMAData DMA读操作核心函数
     * @param devPath PCIe设备路径
     * @param length 要读取的长度
     * @param outData 读取到的数据（输出参数）
     * @return 成功返回true，失败返回false
     */
    bool readDMAData(qint64 length, QByteArray& outData);

    // 成员变量
    int m_deviceFd;               // PCIe设备文件描述符（-1表示未打开）
    QString m_devPath;            // 设备路径（默认/dev/pcie_dma）
    QMutex m_dmaMutex;            // DMA操作互斥锁（避免并发访问）
    DMAStatus m_currentStatus;    // 当前DMA状态
    QTimer* m_statusTimer;        // 状态更新定时器
};

#endif // PCIEDMAWIDGET_H
