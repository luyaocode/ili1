#include "pciedmawidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QGroupBox>
#include <QSpinBox>
#include <QDateTime>
#include <QDebug>

/**
 * @brief PCIeDMAWidget 构造函数
 * @param parent 父窗口
 */
PCIeDMAWidget::PCIeDMAWidget(QWidget *parent)
    : QWidget(parent)
    , m_deviceFd(-1)
    , m_devPath("/dev/pcie_fpga")
    , m_currentStatus(DMA_Idle)
    , m_statusTimer(new QTimer(this))
{
    // 初始化UI和信号槽
    initUI();
    initConnections();

    // 状态定时器：100ms更新一次状态显示
    m_statusTimer->setInterval(100);
    m_statusTimer->start();
}

/**
 * @brief PCIeDMAWidget 析构函数
 */
PCIeDMAWidget::~PCIeDMAWidget()
{
    // 析构时关闭设备
    if (m_deviceFd >= 0) {
        ::close(m_deviceFd);
        m_deviceFd = -1;
    }
}

/**
 * @brief 初始化UI界面
 */
void PCIeDMAWidget::initUI()
{
    // 设置窗口属性
    this->setWindowTitle("PCIe DMA Demo (Qt5.9 + C++11)");
    this->setMinimumSize(800, 600);

    // ========== 设备配置区域 ==========
    QGroupBox* devGroup = new QGroupBox("设备配置", this);
    QHBoxLayout* devLayout = new QHBoxLayout(devGroup);

    QLabel* devPathLabel = new QLabel("设备路径：", this);
    QLineEdit* devPathEdit = new QLineEdit(m_devPath, this);
    devPathEdit->setObjectName("devPathEdit");

    QPushButton* btnOpen = new QPushButton("打开设备", this);
    btnOpen->setObjectName("btnOpenDevice");
    QPushButton* btnClose = new QPushButton("关闭设备", this);
    btnClose->setObjectName("btnCloseDevice");
    btnClose->setEnabled(false);

    devLayout->addWidget(devPathLabel);
    devLayout->addWidget(devPathEdit);
    devLayout->addStretch();
    devLayout->addWidget(btnOpen);
    devLayout->addWidget(btnClose);

    // ========== DMA操作区域 ==========
    QGroupBox* dmaGroup = new QGroupBox("DMA操作", this);
    QVBoxLayout* dmaLayout = new QVBoxLayout(dmaGroup);

    // 写操作子区域
    QHBoxLayout* writeLayout = new QHBoxLayout();
    QLabel* writeDataLabel = new QLabel("写数据：", this);
    QLineEdit* writeDataEdit = new QLineEdit("PCIe DMA Test Data", this);
    writeDataEdit->setObjectName("writeDataEdit");

    QLabel* writeLenLabel = new QLabel("长度：", this);
    QSpinBox* writeLenSpin = new QSpinBox(this);
    writeLenSpin->setRange(1, 4096);
    writeLenSpin->setValue(18);
    writeLenSpin->setObjectName("writeLenSpin");

    QPushButton* btnWrite = new QPushButton("DMA写", this);
    btnWrite->setObjectName("btnDMAWrite");
    btnWrite->setEnabled(false);

    writeLayout->addWidget(writeDataLabel);
    writeLayout->addWidget(writeDataEdit);
    writeLayout->addWidget(writeLenLabel);
    writeLayout->addWidget(writeLenSpin);
    writeLayout->addStretch();
    writeLayout->addWidget(btnWrite);

    // 读操作子区域
    QHBoxLayout* readLayout = new QHBoxLayout();
    QLabel* readLenLabel = new QLabel("读长度：", this);
    QSpinBox* readLenSpin = new QSpinBox(this);
    readLenSpin->setRange(1, 4096);
    readLenSpin->setValue(18);
    readLenSpin->setObjectName("readLenSpin");

    QPushButton* btnRead = new QPushButton("DMA读", this);
    btnRead->setObjectName("btnDMARead");
    btnRead->setEnabled(false);

    readLayout->addWidget(readLenLabel);
    readLayout->addWidget(readLenSpin);
    readLayout->addStretch();
    readLayout->addWidget(btnRead);

    dmaLayout->addLayout(writeLayout);
    dmaLayout->addLayout(readLayout);

    // ========== 状态显示区域 ==========
    QGroupBox* statusGroup = new QGroupBox("操作日志", this);
    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);

    QLabel* statusLabel = new QLabel("当前状态：空闲", this);
    statusLabel->setObjectName("statusLabel");
    statusLabel->setStyleSheet("color: green; font-weight: bold;");

    QTextEdit* logEdit = new QTextEdit(this);
    logEdit->setObjectName("logEdit");
    logEdit->setReadOnly(true);
    logEdit->append("=== PCIe DMA Demo 启动 ===");
    logEdit->append("提示：请先打开PCIe设备（/dev/pcie_dma）");

    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(logEdit);

    // ========== 主布局 ==========
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(devGroup);
    mainLayout->addWidget(dmaGroup);
    mainLayout->addWidget(statusGroup);
    mainLayout->setStretch(0, 1);
    mainLayout->setStretch(1, 1);
    mainLayout->setStretch(2, 3);

    this->setLayout(mainLayout);
}

/**
 * @brief 初始化信号槽连接
 */
void PCIeDMAWidget::initConnections()
{
    // 设备操作按钮
    connect(findChild<QPushButton*>("btnOpenDevice"), &QPushButton::clicked,
            this, &PCIeDMAWidget::onBtnOpenDeviceClicked);
    connect(findChild<QPushButton*>("btnCloseDevice"), &QPushButton::clicked,
            this, &PCIeDMAWidget::onBtnCloseDeviceClicked);

    // DMA操作按钮
    connect(findChild<QPushButton*>("btnDMAWrite"), &QPushButton::clicked,
            this, &PCIeDMAWidget::onBtnDMAWriteClicked);
    connect(findChild<QPushButton*>("btnDMARead"), &QPushButton::clicked,
            this, &PCIeDMAWidget::onBtnDMAReadClicked);

    // 状态定时器
    connect(m_statusTimer, &QTimer::timeout, this, [this]() {
        QString statusStr;
        switch (m_currentStatus) {
        case DMA_Idle: statusStr = "空闲"; break;
        case DMA_Writing: statusStr = "正在写"; break;
        case DMA_Reading: statusStr = "正在读"; break;
        case DMA_Error: statusStr = "错误"; break;
        case DMA_DeviceBusy: statusStr = "设备忙"; break;
        }
        findChild<QLabel*>("statusLabel")->setText(QString("当前状态：%1").arg(statusStr));
    });
}

/**
 * @brief 打开设备按钮点击事件
 */
void PCIeDMAWidget::onBtnOpenDeviceClicked()
{
    // 获取设备路径
    m_devPath = findChild<QLineEdit*>("devPathEdit")->text().trimmed();
    QTextEdit* logEdit = findChild<QTextEdit*>("logEdit");

    // 加锁避免并发操作
    QMutexLocker locker(&m_dmaMutex);

    // 关闭已有设备
    if (m_deviceFd >= 0) {
        ::close(m_deviceFd);
        m_deviceFd = -1;
    }

    // 打开设备（O_RDWR：读写模式，O_NONBLOCK：非阻塞）
    m_deviceFd = open(m_devPath.toUtf8().constData(), O_RDWR | O_NONBLOCK);
    if (m_deviceFd < 0) {
        QString errMsg = QString("打开设备失败：%1（错误码：%2）")
                .arg(strerror(errno)).arg(errno);
        logEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString()).arg(errMsg));
        updateDMAStatus(DMA_Error, errMsg);
        QMessageBox::critical(this, "错误", errMsg);
        return;
    }

    // 打开成功
    QString successMsg = QString("设备 %1 打开成功（fd：%2）").arg(m_devPath).arg(m_deviceFd);
    logEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString()).arg(successMsg));
    updateDMAStatus(DMA_Idle, successMsg);

    // 启用操作按钮
    findChild<QPushButton*>("btnCloseDevice")->setEnabled(true);
    findChild<QPushButton*>("btnDMAWrite")->setEnabled(true);
    findChild<QPushButton*>("btnDMARead")->setEnabled(true);
    findChild<QPushButton*>("btnOpenDevice")->setEnabled(false);
}

/**
 * @brief 关闭设备按钮点击事件
 */
void PCIeDMAWidget::onBtnCloseDeviceClicked()
{
    QMutexLocker locker(&m_dmaMutex);
    QTextEdit* logEdit = findChild<QTextEdit*>("logEdit");

    if (m_deviceFd >= 0) {
        ::close(m_deviceFd);
        m_deviceFd = -1;
        QString msg = QString("设备 %1 已关闭").arg(m_devPath);
        logEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString()).arg(msg));
        updateDMAStatus(DMA_Idle, msg);
    }

    // 禁用操作按钮
    findChild<QPushButton*>("btnCloseDevice")->setEnabled(false);
    findChild<QPushButton*>("btnDMAWrite")->setEnabled(false);
    findChild<QPushButton*>("btnDMARead")->setEnabled(false);
    findChild<QPushButton*>("btnOpenDevice")->setEnabled(true);
}

/**
 * @brief DMA写操作按钮点击事件
 */
void PCIeDMAWidget::onBtnDMAWriteClicked()
{
    // 异步执行DMA写操作（避免阻塞UI）
    QTimer::singleShot(0, this, [this]() {
        QMutexLocker locker(&m_dmaMutex);
        QTextEdit* logEdit = findChild<QTextEdit*>("logEdit");

        // 检查设备是否打开
        if (m_deviceFd < 0) {
            handleDMAError("设备未打开");
            return;
        }

        // 获取写数据和长度
        QString writeData = findChild<QLineEdit*>("writeDataEdit")->text();
        qint64 writeLen = findChild<QSpinBox*>("writeLenSpin")->value();
        QByteArray data = writeData.toUtf8().left(writeLen);

        // 更新状态
        updateDMAStatus(DMA_Writing, "开始DMA写操作");
        logEdit->append(QString("[%1] 执行DMA写：数据=%2，长度=%3")
                        .arg(QDateTime::currentDateTime().toString())
                        .arg(writeData).arg(writeLen));

        // 执行DMA写
        bool ret = writeDMAData(data, writeLen);
        if (ret) {
            QString successMsg = "DMA写操作成功";
            logEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString()).arg(successMsg));
            updateDMAStatus(DMA_Idle, successMsg);
        } else {
            QString errMsg = QString("DMA写操作失败：%1（错误码：%2）").arg(strerror(errno)).arg(errno);
            logEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString()).arg(errMsg));
            if (errno == EBUSY) {
                updateDMAStatus(DMA_DeviceBusy, errMsg);
            } else {
                updateDMAStatus(DMA_Error, errMsg);
            }
            QMessageBox::warning(this, "警告", errMsg);
        }
    });
}

/**
 * @brief DMA读操作按钮点击事件
 */
void PCIeDMAWidget::onBtnDMAReadClicked()
{
    QTimer::singleShot(0, this, [this]() {
        QMutexLocker locker(&m_dmaMutex);
        QTextEdit* logEdit = findChild<QTextEdit*>("logEdit");

        if (m_deviceFd < 0) {
            handleDMAError("设备未打开");
            return;
        }

        // 获取读长度
        qint64 readLen = findChild<QSpinBox*>("readLenSpin")->value();
        QByteArray readData;

        // 更新状态
        updateDMAStatus(DMA_Reading, "开始DMA读操作");
        logEdit->append(QString("[%1] 执行DMA读：长度=%2")
                        .arg(QDateTime::currentDateTime().toString()).arg(readLen));

        // 执行DMA读
        bool ret = readDMAData(readLen, readData);
        if (ret) {
            QString successMsg = QString("DMA读操作成功，数据：%1").arg(QString::fromUtf8(readData));
            logEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString()).arg(successMsg));
            updateDMAStatus(DMA_Idle, successMsg);
            QMessageBox::information(this, "成功", successMsg);
        } else {
            QString errMsg = QString("DMA读操作失败：%1（错误码：%2）").arg(strerror(errno)).arg(errno);
            logEdit->append(QString("[%1] %2").arg(QDateTime::currentDateTime().toString()).arg(errMsg));
            if (errno == EBUSY) {
                updateDMAStatus(DMA_DeviceBusy, errMsg);
            } else {
                updateDMAStatus(DMA_Error, errMsg);
            }
            QMessageBox::warning(this, "警告", errMsg);
        }
    });
}

/**
 * @brief 更新DMA状态
 * @param status 新状态
 * @param msg 状态描述
 */
void PCIeDMAWidget::updateDMAStatus(DMAStatus status, const QString& msg)
{
    m_currentStatus = status;
    QLabel* statusLabel = findChild<QLabel*>("statusLabel");

    // 根据状态设置颜色
    switch (status) {
    case DMA_Idle:
        statusLabel->setStyleSheet("color: green; font-weight: bold;");
        break;
    case DMA_Writing:
    case DMA_Reading:
        statusLabel->setStyleSheet("color: blue; font-weight: bold;");
        break;
    case DMA_Error:
        statusLabel->setStyleSheet("color: red; font-weight: bold;");
        break;
    case DMA_DeviceBusy:
        statusLabel->setStyleSheet("color: orange; font-weight: bold;");
        break;
    }
}

/**
 * @brief 处理DMA错误
 * @param errMsg 错误信息
 */
void PCIeDMAWidget::handleDMAError(const QString& errMsg)
{
    updateDMAStatus(DMA_Error, errMsg);
    QTextEdit* logEdit = findChild<QTextEdit*>("logEdit");
    logEdit->append(QString("[%1] 错误：%2").arg(QDateTime::currentDateTime().toString()).arg(errMsg));
    QMessageBox::critical(this, "错误", errMsg);
}

/**
 * @brief DMA写操作核心实现
 * @param devPath 设备路径
 * @param data 要写入的数据
 * @param length 数据长度
 * @return 成功返回true，失败返回false
 */
bool PCIeDMAWidget::writeDMAData(const QByteArray& data, qint64 length)
{
    if (m_deviceFd < 0) {
        errno = EBADF;
        return false;
    }

    // 实际DMA写操作：向设备节点写入数据
    ssize_t ret = write(m_deviceFd, data.constData(), length);
    if (ret < 0) {
        return false;
    }

    // 检查写入长度是否匹配
    if (ret != length) {
        errno = EIO;
        return false;
    }

    return true;
}

/**
 * @brief DMA读操作核心实现
 * @param devPath 设备路径
 * @param length 读取长度
 * @param outData 输出数据
 * @return 成功返回true，失败返回false
 */
bool PCIeDMAWidget::readDMAData(qint64 length, QByteArray& outData)
{
    if (m_deviceFd < 0) {
        errno = EBADF;
        qDebug() << "错误：设备文件描述符无效，fd=" << m_deviceFd;
        return false;
    }

    // 清空输出数据
    outData.clear();
    // 分配缓冲区（避免频繁 new/delete）
    char* buf = new char[length];
    memset(buf, 0, length);

    qint64 totalRead = 0; // 累计读取字节数
    while (totalRead < length) {
        // 读取剩余未读的字节数
        ssize_t ret = read(m_deviceFd, buf + totalRead, length - totalRead);

        if (ret < 0) {
            // 处理读取出错（区分“中断重试”和“真错误”）
            if (errno == EINTR) {
                // 被信号中断，继续读取
                continue;
            } else {
                qDebug() << "读取失败，错误码：" << errno << "，错误描述：" << strerror(errno);
                delete[] buf;
                return false;
            }
        } else if (ret == 0) {
            // 读到末尾，数据不足
            qDebug() << "读取到末尾，仅读取" << totalRead << "字节，期望" << length << "字节";
            break;
        }

        // 累计读取长度
        totalRead += ret;
        qDebug() << "单次读取" << ret << "字节，累计" << totalRead << "字节";
    }

    // 保存完整数据
    outData = QByteArray(buf, totalRead);
    delete[] buf;

    return true;
}
