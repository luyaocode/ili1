#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <QGroupBox>

MainWindow::MainWindow(QWidget *parent): QWidget(parent)
{
    // 1. 初始化UI
    setWindowTitle("Linux MMAP Demo (Qt5.9.5 + C++11) - 自定义配置");
    setFixedSize(700, 500);

    // 整体布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // ---------------------- 新增：MMAP配置区域 ----------------------
    QGroupBox   *configGroup  = new QGroupBox("MMAP配置", this);
    QGridLayout *configLayout = new QGridLayout(configGroup);

    // 1.1 文件路径选择
    QLabel *pathLabel = new QLabel("文件路径：", this);
    m_filePathEdit    = new QLineEdit(this);
    m_filePathEdit->setPlaceholderText("请输入文件存储路径（如/opt/）");
    m_filePathEdit->setText("./");  // 默认路径
    m_selectPathBtn = new QPushButton("选择路径", this);
    configLayout->addWidget(pathLabel, 0, 0);
    configLayout->addWidget(m_filePathEdit, 0, 1);
    configLayout->addWidget(m_selectPathBtn, 0, 2);

    // 1.2 文件名输入
    QLabel *nameLabel = new QLabel("文件名：", this);
    m_fileNameEdit    = new QLineEdit(this);
    m_fileNameEdit->setPlaceholderText("请输入文件名（如mmap_data.dat）");
    m_fileNameEdit->setText("mmap_data.dat");  // 默认文件名
    configLayout->addWidget(nameLabel, 1, 0);
    configLayout->addWidget(m_fileNameEdit, 1, 1, 1, 2);

    // 1.3 映射大小设置（单位：字节，范围1024~1048576）
    QLabel *sizeLabel = new QLabel("映射大小（字节）：", this);
    m_mapSizeSpinBox  = new QSpinBox(this);
    m_mapSizeSpinBox->setRange(1024, 1048576);  // 1KB ~ 1MB
    m_mapSizeSpinBox->setValue(1024);           // 默认1024字节
    m_reinitMmapBtn = new QPushButton("应用配置（重新初始化MMAP）", this);
    configLayout->addWidget(sizeLabel, 2, 0);
    configLayout->addWidget(m_mapSizeSpinBox, 2, 1);
    configLayout->addWidget(m_reinitMmapBtn, 2, 2);

    mainLayout->addWidget(configGroup);

    // ---------------------- 原有：数据操作区域 ----------------------
    QGroupBox   *operateGroup  = new QGroupBox("数据操作", this);
    QVBoxLayout *operateLayout = new QVBoxLayout(operateGroup);

    // 输入框：用于输入要存储的数据
    m_textEdit = new QTextEdit(this);
    m_textEdit->setPlaceholderText("请输入要存储的测试数据（模拟图片/范围存储数据）");
    operateLayout->addWidget(m_textEdit);

    // 按钮布局
    QHBoxLayout *btnLayout = new QHBoxLayout();
    // 按钮：写入MMAP
    QPushButton *btnWrite = new QPushButton("写入MMAP（无write调用）", this);
    // 按钮：读取MMAP
    QPushButton *btnRead = new QPushButton("读取MMAP（无read调用）", this);
    // 按钮：手动刷盘
    QPushButton *btnFlush = new QPushButton("手动刷盘（同步到磁盘）", this);
    btnLayout->addWidget(btnWrite);
    btnLayout->addWidget(btnRead);
    btnLayout->addWidget(btnFlush);
    operateLayout->addLayout(btnLayout);

    mainLayout->addWidget(operateGroup);

    // ---------------------- 初始化MMAP（默认配置） ----------------------
    if (!m_mmap.init(getFullFilePath(), m_mapSizeSpinBox->value()))
    {
        QMessageBox::critical(this, "错误", "MMAP初始化失败！");
        close();
        return;
    }

    // ---------------------- 绑定信号槽 ----------------------
    // 原有槽函数
    connect(btnWrite, &QPushButton::clicked, this, &MainWindow::onWriteClicked);
    connect(btnRead, &QPushButton::clicked, this, &MainWindow::onReadClicked);
    connect(btnFlush, &QPushButton::clicked, this, &MainWindow::onFlushClicked);
    // 新增槽函数
    connect(m_selectPathBtn, &QPushButton::clicked, this, &MainWindow::onSelectPathClicked);
    connect(m_reinitMmapBtn, &QPushButton::clicked, this, &MainWindow::onReinitMmapClicked);
}

MainWindow::~MainWindow()
{
}

// 新增：选择文件路径
void MainWindow::onSelectPathClicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择存储路径", "./");
    if (!dir.isEmpty())
    {
        m_filePathEdit->setText(dir + "/");  // 补充路径分隔符
    }
}

// 新增：重新初始化MMAP（应用自定义配置）
void MainWindow::onReinitMmapClicked()
{
    // 1. 先解除原有MMAP映射
    m_mmap.unmap();

    // 2. 获取用户配置的路径、文件名、大小
    QString fullPath = getFullFilePath();
    qint64  mapSize  = m_mapSizeSpinBox->value();

    // 3. 重新初始化MMAP
    if (m_mmap.init(fullPath, mapSize))
    {
        QMessageBox::information(
            this, "成功", QString("MMAP重新初始化成功！\n文件路径：%1\n映射大小：%2字节").arg(fullPath).arg(mapSize));
    }
    else
    {
        QMessageBox::critical(
            this, "失败", QString("MMAP重新初始化失败！\n文件路径：%1\n映射大小：%2字节").arg(fullPath).arg(mapSize));
    }
}

// 辅助函数：拼接完整文件路径
QString MainWindow::getFullFilePath()
{
    QString path = m_filePathEdit->text().trimmed();
    QString name = m_fileNameEdit->text().trimmed();

    // 处理路径分隔符
    if (!path.endsWith("/"))
    {
        path += "/";
    }

    return path + name;
}

// 原有：写入按钮点击事件
void MainWindow::onWriteClicked()
{
    QString input = m_textEdit->toPlainText();
    if (input.isEmpty())
    {
        QMessageBox::warning(this, "提示", "请输入要写入的数据！");
        return;
    }

    // 校验MMAP是否已初始化
    if (!m_mmap.isInitialized())
    {
        QMessageBox::warning(this, "提示", "MMAP未初始化，请先点击「应用配置」！");
        return;
    }

    // 写入到MMAP内存（偏移量0，覆盖开头）
    bool ret = m_mmap.writeData(input.toUtf8(), 0);
    if (ret)
    {
        QMessageBox::information(this, "成功", "数据已写入MMAP内存（系统会异步刷到磁盘）！");
    }
    else
    {
        QMessageBox::critical(this, "失败", "数据写入失败！");
    }
}

// 原有：读取按钮点击事件
void MainWindow::onReadClicked()
{
    // 校验MMAP是否已初始化
    if (!m_mmap.isInitialized())
    {
        QMessageBox::warning(this, "提示", "MMAP未初始化，请先点击「应用配置」！");
        return;
    }

    // 读取偏移量0开始的全部映射区域
    QByteArray data = m_mmap.readData(0, m_mmap.mapSize());
    if (data.isEmpty())
    {
        QMessageBox::warning(this, "提示", "读取失败或无数据！");
        return;
    }

    // 显示读取结果（过滤空字符）
    QString result = QString::fromUtf8(data).trimmed();
    m_textEdit->setPlainText(result);
}

// 原有：刷盘按钮点击事件
void MainWindow::onFlushClicked()
{
    // 校验MMAP是否已初始化
    if (!m_mmap.isInitialized())
    {
        QMessageBox::warning(this, "提示", "MMAP未初始化，请先点击「应用配置」！");
        return;
    }

    bool ret = m_mmap.flush();
    if (ret)
    {
        QMessageBox::information(this, "成功", "已强制将MMAP内存数据同步到磁盘！");
    }
    else
    {
        QMessageBox::critical(this, "失败", "刷盘失败！");
    }
}
