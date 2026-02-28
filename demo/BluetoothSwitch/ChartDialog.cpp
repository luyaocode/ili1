#include "ChartDialog.h"

ChartDialog::ChartDialog(QWidget *parent): QDialog(parent)
{
    // 设置对话框属性
    this->setWindowTitle("蓝牙扫描耗时分析");
    this->resize(1200, 800);
    this->setModal(true);  // 模态对话框，阻塞主窗口

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // 1. 按钮栏
    QWidget     *btnWidget = new QWidget(this);
    QHBoxLayout *btnLayout = new QHBoxLayout(btnWidget);
    mainLayout->addWidget(btnWidget);

    m_btnSelectLog = new QPushButton("选择日志文件", btnWidget);
    m_btnSaveChart = new QPushButton("保存图表", btnWidget);
    m_btnSaveChart->setEnabled(false);  // 初始禁用

    btnLayout->addWidget(m_btnSelectLog);
    btnLayout->addWidget(m_btnSaveChart);
    btnLayout->addStretch();

    // 2. 初始化图表
    initChart();
    mainLayout->addWidget(m_chartView);

    // 绑定按钮信号
    connect(m_btnSelectLog, &QPushButton::clicked, this, &ChartDialog::onSelectLogFile);
    connect(m_btnSaveChart, &QPushButton::clicked, this, &ChartDialog::onSaveChart);
}

ChartDialog::~ChartDialog()
{
    // 释放图表资源
    if (m_chart)
        delete m_chart;
    if (m_series)
        delete m_series;
}

void ChartDialog::initChart()
{
    // 创建图表核心对象
    m_chart = new QChart();
    m_chart->setTitle("蓝牙startScan耗时随时间变化曲线");
    m_chart->setAnimationOptions(QChart::SeriesAnimations);

    // 创建折线系列
    m_series = new QLineSeries();
    m_series->setName("耗时(ms)");
    m_chart->addSeries(m_series);

    // ========== 修复坐标轴创建&绑定逻辑 ==========
    // 配置X轴（时间轴）
    QDateTimeAxis *axisX = new QDateTimeAxis();
    axisX->setFormat("HH:mm:ss");
    axisX->setTitleText("时间");
    axisX->setTickCount(10);
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_series->attachAxis(axisX);  // 系列绑定X轴

    // 配置Y轴（数值轴）
    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("startScan耗时(ms)");
    axisY->setLabelFormat("%d");
    m_chart->addAxis(axisY, Qt::AlignLeft);
    m_series->attachAxis(axisY);  // 系列绑定Y轴

    // 创建图表视图
    m_chartView = new QChartView(m_chart);
    m_chartView->setRenderHint(QPainter::Antialiasing);
    m_chartView->setMinimumHeight(600);
    // 关键：确保视图显示完整图表
    m_chartView->setResizeAnchor(QChartView::AnchorViewCenter);
}

void ChartDialog::onSelectLogFile()
{
    QString logPath =
        QFileDialog::getOpenFileName(this, "选择日志文件", QDir::homePath(), "日志文件 (*.log *.txt);;所有文件 (*.*)");
    if (logPath.isEmpty())
        return;

    m_logData = parseLogFile(logPath);
    if (m_logData.isEmpty())
    {
        QMessageBox::warning(this, "警告", "未解析到有效日志数据！");
        m_btnSaveChart->setEnabled(false);
        return;
    }

    m_series->clear();
    for (const LogData &data : m_logData)
    {
        m_series->append(data.time.toMSecsSinceEpoch(), data.elapsedMs);
    }

    QList<QAbstractAxis *> axes = m_chart->axes();
    for (QAbstractAxis *axis : axes)
    {
        // 正确枚举值：AxisTypeDateTime（带AxisType前缀）
        if (axis->type() == QAbstractAxis::AxisTypeDateTime)
        {
            QDateTimeAxis *axisX = qobject_cast<QDateTimeAxis *>(axis);
            if (axisX)
            {
                axisX->setRange(m_logData.first().time, m_logData.last().time);
            }
        }
        // 正确枚举值：AxisTypeValue（带AxisType前缀）
        else if (axis->type() == QAbstractAxis::AxisTypeValue)
        {
            QValueAxis *axisY = qobject_cast<QValueAxis *>(axis);
            if (axisY)
            {
                // 手动计算Y轴最大值
                qint64 maxElapsed = 0;
                for (const LogData &data : m_logData)
                {
                    if (data.elapsedMs > maxElapsed)
                        maxElapsed = data.elapsedMs;
                }
                axisY->setRange(0, maxElapsed * 1.1);
            }
        }
    }

    m_btnSaveChart->setEnabled(true);
    QMessageBox::information(this, "成功", QString("解析完成！共加载 %1 条数据").arg(m_logData.size()));

    // 强制刷新图表视图
    m_chartView->repaint();
}

void ChartDialog::onSaveChart()
{
    if (m_logData.isEmpty())
        return;

    // 默认保存路径：用户主目录 + 时间戳文件名
    QString defaultFileName =
        QString("蓝牙耗时图表_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    QString defaultPath = QDir::homePath() + "/" + defaultFileName;

    // 选择保存路径（默认填充上述路径）
    QString savePath =
        QFileDialog::getSaveFileName(this, "保存图表", defaultPath, "PNG图片 (*.png);;JPG图片 (*.jpg *.jpeg)");
    if (savePath.isEmpty())
        return;

    QPixmap pixmap(m_chartView->size());
    pixmap.fill(Qt::white);  // 填充白色背景（避免透明背景）

    // 2. 创建QPainter并关联pixmap
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);  // 抗锯齿

    // 3. 渲染图表视图到pixmap（QGraphicsView的正确render方式）
    m_chartView->render(&painter);

    // 4. 结束绘画并保存
    painter.end();
    if (pixmap.save(savePath))
    {
        QMessageBox::information(this, "成功", QString("图表已保存至：\n%1").arg(savePath));
    }
    else
    {
        QMessageBox::critical(this, "失败", "图表保存失败！请检查路径权限。");
    }
}

QVector<LogData> ChartDialog::parseLogFile(const QString &filePath)
{
    QVector<LogData> logDataList;
    QFile            file(filePath);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, "错误", QString("无法打开日志文件：\n%1").arg(filePath));
        return logDataList;
    }

    QTextStream in(&file);
    // 正则匹配时间和耗时
    QRegExp timeRegex("(\\d{4}-\\d{2}-\\d{2} \\d{2}:\\d{2}:\\d{2}\\.\\d{3})");
    QRegExp elapsedRegex("startScan elapsed: (\\d+) ms");

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;

        // 提取时间
        int timePos = timeRegex.indexIn(line);
        if (timePos == -1)
            continue;
        QString   timeStr = timeRegex.cap(1);
        QDateTime time    = QDateTime::fromString(timeStr, "yyyy-MM-dd HH:mm:ss.zzz");
        if (!time.isValid())
            continue;

        // 提取耗时
        int elapsedPos = elapsedRegex.indexIn(line);
        if (elapsedPos == -1)
            continue;
        qint64 elapsedMs = elapsedRegex.cap(1).toLongLong();

        LogData data;
        data.time      = time;
        data.elapsedMs = elapsedMs;
        logDataList.append(data);
    }

    file.close();
    return logDataList;
}
