#include "transformer_demo_window.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QBrush>
#include <QPen>
#include <QLineSeries>
#include <QApplication>
#include <QMessageBox>
#include <cmath>
#include <iostream>  // 用于终端输出日志
#include <iomanip>

TransformerDemoWindow::TransformerDemoWindow(QWidget *parent): QMainWindow(parent)
{
    // 初始化Transformer编码器
    m_transformer = new TransformerEncoder(64, 8, 256, 3);  // 8头、d_ff=256、3层编码器

    // 窗口基础设置
    setWindowTitle("Transformer演示程序 (C++11 + Qt5.9)");
    setFixedSize(1200, 800);  // 加宽窗口适配左右布局

    // 中心部件
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 主布局（垂直）
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 运行+训练按钮区域
    QWidget *btnWidget = new QWidget(this);
    QHBoxLayout *btnLayout = new QHBoxLayout(btnWidget);
    m_runBtn = new QPushButton("运行Transformer演示", this);
    m_runBtn->setMinimumHeight(40);
    m_epochEdit = new QLineEdit("100", this);
    m_epochEdit->setPlaceholderText("训练轮数");
    m_epochEdit->setFixedWidth(100);
    m_trainBtn = new QPushButton("训练模型", this);
    btnLayout->addWidget(m_runBtn);
    btnLayout->addStretch();  // 间距拉伸
    btnLayout->addWidget(new QLabel("训练轮数："));
    btnLayout->addWidget(m_epochEdit);
    btnLayout->addWidget(m_trainBtn);
    mainLayout->addWidget(btnWidget);

    // ========== 核心调整：两张图表左右布局 ==========
    QWidget *chartWidget = new QWidget(this);
    QHBoxLayout *chartLayout = new QHBoxLayout(chartWidget);
    chartLayout->setSpacing(20);

    // 左图：Loss曲线
    m_lossView = new QChartView(this);
    m_lossView->setMinimumSize(500, 300);
    QChart *lossChart = new QChart();
    lossChart->setTitle("训练损失曲线");
    m_lossView->setChart(lossChart);
    chartLayout->addWidget(m_lossView);

    // 右图：准确率曲线
    m_accView = new QChartView(this);
    m_accView->setMinimumSize(500, 300);
    QChart *accChart = new QChart();
    accChart->setTitle("训练准确率曲线");
    m_accView->setChart(accChart);
    chartLayout->addWidget(m_accView);

    mainLayout->addWidget(chartWidget);

    // ========== 热力图区域（放在图表下方） ==========
    QChart *emptyChart = new QChart();
    emptyChart->setTitle("训练完成后显示注意力热力图");
    m_heatmapView = new QChartView(emptyChart, this);
    m_heatmapView->setMinimumHeight(300);
    m_heatmapView->setRenderHint(QPainter::Antialiasing);
    mainLayout->addWidget(m_heatmapView);

    // 连接信号槽
    connect(m_runBtn, &QPushButton::clicked, this, &TransformerDemoWindow::runDemo);
    connect(m_trainBtn, &QPushButton::clicked, this, &TransformerDemoWindow::trainModel);
}

TransformerDemoWindow::~TransformerDemoWindow()
{
    // 释放内存
    delete m_transformer;

    // 释放Loss图表
    QChart *lossChart = m_lossView->chart();
    if (lossChart) delete lossChart;
    delete m_lossView;

    // 释放准确率图表
    QChart *accChart = m_accView->chart();
    if (accChart) delete accChart;
    delete m_accView;

    // 释放热力图
    QChart *heatmapChart = m_heatmapView->chart();
    if (heatmapChart) delete heatmapChart;
    delete m_heatmapView;
}

void TransformerDemoWindow::runDemo()
{
    m_runBtn->setEnabled(false);
    // 日志输出到终端
    std::cout << "=== 开始运行Transformer演示 ===" << std::endl;

    try
    {
        // 1. 生成测试输入
        Tensor2D input = TransformerUtils::randomInit(8, 64);
        if (input.empty() || input[0].empty())
        {
            throw std::runtime_error("输入序列初始化失败");
        }

        // 2. 运行Transformer编码器
        Tensor2D output = m_transformer->forward(input);
        if (output.empty() || output[0].empty())
        {
            throw std::runtime_error("Transformer编码输出失败");
        }

        // 3. 获取注意力权重
        Tensor3D attnWeights = m_transformer->getLastLayerAttentionWeights();
        if (attnWeights.empty() || attnWeights[0].empty())
        {
            throw std::runtime_error("注意力权重获取失败");
        }

        // 4. 终端输出基础信息
        std::cout << "输入序列维度：" << input.size() << " × " << input[0].size() << std::endl;
        std::cout << "输出序列维度：" << output.size() << " × " << output[0].size() << std::endl;
        std::cout << "注意力头数：" << attnWeights.size() << std::endl;
        std::cout << "单头注意力权重维度：" << attnWeights[0].size() << " × " << attnWeights[0][0].size() << std::endl;
        std::cout << "说明：伪热力图展示第一个注意力头的权重分布，颜色越红/点越大表示注意力权重越高" << std::endl;

        // 5. 绘制注意力伪热力图
        plotAttentionHeatmap(attnWeights);
    }
    catch (const std::runtime_error &e)
    {
        QMessageBox::critical(this, "运行错误", QString("错误：%1").arg(e.what()));
        std::cerr << "运行失败：" << e.what() << std::endl;
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(this, "运行错误", QString("未知异常：%1").arg(e.what()));
        std::cerr << "运行失败：" << e.what() << std::endl;
    }
    catch (...)
    {
        QMessageBox::critical(this, "运行错误", "发生未知错误");
        std::cerr << "运行失败：未知错误" << std::endl;
    }

    m_runBtn->setEnabled(true);
}

void TransformerDemoWindow::plotAttentionHeatmap(const Tensor3D &attnWeights)
{
    // 清除旧图表
    QChart *oldChart = m_heatmapView->chart();
    if (oldChart)
    {
        oldChart->removeAllSeries();
        oldChart->deleteLater();
    }

    QChart *chart = new QChart();
    chart->setTitle("Transformer 注意力权重热力图");
    chart->setAnimationOptions(QChart::NoAnimation);
    chart->legend()->hide();

    const Tensor2D &attn    = attnWeights[0];
    int             seq_len = attn.size();

    // 找最大最小值，用于归一化
    float min_v = 1e9;
    float max_v = -1e9;
    for (int i = 0; i < seq_len; ++i)
    {
        for (int j = 0; j < seq_len; ++j)
        {
            min_v = qMin(min_v, attn[i][j]);
            max_v = qMax(max_v, attn[i][j]);
        }
    }
    float range = max_v - min_v;
    if (range < 1e-6)
        range = 1e-6;

    // 每个点创建一个独立散点序列
    for (int i = 0; i < seq_len; ++i)
    {
        for (int j = 0; j < seq_len; ++j)
        {
            float val  = attn[i][j];
            float norm = (val - min_v) / range;

            QScatterSeries *s = new QScatterSeries();
            s->append(j, i);

            // 颜色：蓝(低权重) → 红(高权重)
            int    hue = 240 - norm * 240;
            QColor c   = QColor::fromHsv(hue, 255, 255);

            s->setBrush(c);
            s->setPen(QPen(Qt::black, 1));
            s->setMarkerSize(5 + norm * 20);  // 权重越大，点越大

            chart->addSeries(s);
        }
    }

    // 创建坐标轴
    QValueAxis *axisX = new QValueAxis();
    QValueAxis *axisY = new QValueAxis();
    axisX->setRange(-0.5, seq_len - 0.5);
    axisY->setRange(-0.5, seq_len - 0.5);
    axisX->setTickCount(seq_len + 1);
    axisY->setTickCount(seq_len + 1);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    for (auto *s : chart->series())
    {
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }

    m_heatmapView->setChart(chart);
}

void TransformerDemoWindow::trainModel()
{
    m_trainBtn->setEnabled(false);
    m_lossHistory.clear();
    m_accHistory.clear();

    // 终端输出训练开始日志
    std::cout << "\n=== 新轮次训练开始 - 序列排序任务 ===" << std::endl;

    // 1. 核心参数
    int target_col = 1;
    int epochs     = m_epochEdit->text().toInt();
    if (epochs <= 0) epochs = 200;
    float init_lr  = 0.005f;
    int   seq_len  = 8;
    int   d_model  = 64;
    int   batch_size = 8;

    // 2. 生成测试集
    auto test_batch = TransformerUtils::createBatchSortingIndexTasks(10, seq_len, d_model, target_col);

    // 3. 终端打印初始信息
    std::cout << "序列长度：" << seq_len << " | 模型维度：" << d_model << std::endl;
    std::cout << "批量大小：" << batch_size << " | 训练轮数：" << epochs << " | 初始学习率：" << init_lr << std::endl;
    std::cout << "测试集样本数：" << test_batch.size() << std::endl;

    // ======================
    // 初始化Loss图表（左图）- 修复横轴范围
    // ======================
    if (m_lossChart) {
        delete m_lossChart;
        m_lossChart = nullptr;
    }
    if (m_lossSeries) {
        delete m_lossSeries;
        m_lossSeries = nullptr;
    }
    m_lossChart = new QChart();
    m_lossChart->setTitle("训练损失曲线 (Loss)");
    m_lossChart->legend()->setVisible(true);
    m_lossSeries = new QLineSeries();
    m_lossSeries->setName("Loss");
    m_lossSeries->setColor(Qt::red);
    m_lossSeries->setPen(QPen(Qt::red, 2));

    // 修复1：显式创建横轴并设置初始范围（匹配总训练轮数）
    QValueAxis *lossAxisX = new QValueAxis();
    QValueAxis *lossAxisY = new QValueAxis();
    lossAxisX->setTitleText("Epoch");
    lossAxisX->setRange(0, epochs);  // 横轴初始范围设为总训练轮数
    lossAxisX->setTickCount(11);    // 显示11个刻度（0,20,40...200），更清晰
    lossAxisY->setTitleText("Loss");
    lossAxisY->setRange(0, 10);

    m_lossChart->addAxis(lossAxisX, Qt::AlignBottom);
    m_lossChart->addAxis(lossAxisY, Qt::AlignLeft);
    m_lossChart->addSeries(m_lossSeries);
    m_lossSeries->attachAxis(lossAxisX);
    m_lossSeries->attachAxis(lossAxisY);
    m_lossView->setChart(m_lossChart);
    m_lossView->setRenderHint(QPainter::Antialiasing);

    // ======================
    // 初始化准确率图表（右图）- 修复横轴范围
    // ======================
    if (m_accChart) {
        delete m_accChart;
        m_accChart = nullptr;
    }
    if (m_accSeries) {
        delete m_accSeries;
        m_accSeries = nullptr;
    }
    m_accChart = new QChart();
    m_accChart->setTitle("训练准确率曲线 (Accuracy)");
    m_accChart->legend()->setVisible(true);
    m_accSeries = new QLineSeries();
    m_accSeries->setName("准确率(%)");
    m_accSeries->setColor(Qt::blue);
    m_accSeries->setPen(QPen(Qt::blue, 2));

    // 修复1：显式创建横轴并设置初始范围（匹配总训练轮数）
    QValueAxis *accAxisX = new QValueAxis();
    QValueAxis *accAxisY = new QValueAxis();
    accAxisX->setTitleText("Epoch");
    accAxisX->setRange(0, epochs);  // 横轴初始范围设为总训练轮数
    accAxisX->setTickCount(11);     // 显示11个刻度
    accAxisY->setTitleText("Accuracy (%)");
    accAxisY->setRange(0, 100);

    m_accChart->addAxis(accAxisX, Qt::AlignBottom);
    m_accChart->addAxis(accAxisY, Qt::AlignLeft);
    m_accChart->addSeries(m_accSeries);
    m_accSeries->attachAxis(accAxisX);
    m_accSeries->attachAxis(accAxisY);
    m_accView->setChart(m_accChart);
    m_accView->setRenderHint(QPainter::Antialiasing);

    // 强制刷新UI
    QApplication::processEvents();

    // 4. 训练循环
    for (int epoch = 0; epoch < epochs; ++epoch)
    {
        float lr = init_lr * (1.0f - (float)epoch / epochs);
        auto train_batch = TransformerUtils::createBatchSortingIndexTasks(batch_size, seq_len, d_model, target_col);

        float total_loss = 0.0f;
        float total_acc  = 0.0f;
        std::vector<Tensor2D> train_preds;

        for (const auto &sample : train_batch)
        {
            auto input  = sample.first;
            auto target = sample.second;
            Tensor2D pred = m_transformer->forward(input);
            train_preds.push_back(pred);

            // 计算Loss
            float loss = 0.0f;
            for (int i = 0; i < seq_len; ++i) {
                float diff = pred[i][0] - target[i][0];
                loss += diff * diff;
            }
            loss /= seq_len;
            total_loss += loss;

            // 计算准确率
            float acc = 0.0f;
            for (int i = 0; i < seq_len; ++i) {
                if (fabs(pred[i][0] - target[i][0]) < 0.5f) {
                    acc += 1.0f;
                }
            }
            acc /= seq_len;
            total_acc += acc;

            // 反向传播
            Tensor2D d_out(seq_len, std::vector<float>(d_model, 0.0f));
            for (int i = 0; i < seq_len; ++i) {
                d_out[i][0] = 100.0f * (pred[i][0] - target[i][0]);
            }
            m_transformer->backward(d_out);
            m_transformer->updateParams(lr);
            m_transformer->clearAllGrads();
        }

        // 保存本轮指标
        float avg_loss = total_loss / batch_size;
        float avg_acc  = total_acc / batch_size * 100;
        m_lossHistory.push_back(avg_loss);
        m_accHistory.push_back(avg_acc);

        // 每10轮更新日志+曲线
        if (epoch % 10 == 0)
        {
            // 终端输出训练日志
            std::cout << "Epoch " << epoch
                      << " | LR: " << std::fixed << std::setprecision(4) << lr
                      << " | Loss: " << std::fixed << std::setprecision(4) << avg_loss
                      << " | Acc: " << std::fixed << std::setprecision(2) << avg_acc << "%" << std::endl;

            // ======================
            // 修复2：更新Loss曲线 + 强制刷新横轴范围
            // ======================
            m_lossSeries->clear();
            for (int i = 0; i < m_lossHistory.size(); ++i) {
                m_lossSeries->append(i, m_lossHistory[i]);
            }
            // 自适应Loss纵轴
            float max_loss = *std::max_element(m_lossHistory.begin(), m_lossHistory.end());
            static_cast<QValueAxis*>(m_lossChart->axes(Qt::Vertical).first())->setRange(0, max_loss * 1.1);
            // 强制设置横轴范围为 0 ~ 当前最大轮数（避免缩到0-1）
            static_cast<QValueAxis*>(m_lossChart->axes(Qt::Horizontal).first())->setRange(0, epochs);
            m_lossChart->createDefaultAxes();
            m_lossView->repaint();

            // ======================
            // 修复2：更新准确率曲线 + 强制刷新横轴范围
            // ======================
            m_accSeries->clear();
            for (int i = 0; i < m_accHistory.size(); ++i) {
                m_accSeries->append(i, m_accHistory[i]);
            }
            // 强制设置横轴范围为 0 ~ 当前最大轮数
            static_cast<QValueAxis*>(m_accChart->axes(Qt::Horizontal).first())->setRange(0, epochs);
            m_accChart->createDefaultAxes();
            m_accView->repaint();

            // 刷新UI
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        }
    }

    // 5. 测试集评估
    std::cout << "\n=== 训练完成 - 测试集评估 ===" << std::endl;
    float total_test_loss = 0.0f, total_test_acc = 0.0f;
    for (const auto &sample : test_batch) {
        auto input = sample.first;
        auto target = sample.second;
        Tensor2D pred = m_transformer->forward(input);

        float loss = 0.0f, acc = 0.0f;
        for (int i = 0; i < seq_len; ++i) {
            loss += pow(pred[i][0] - target[i][0], 2);
            if (fabs(pred[i][0] - target[i][0]) < 0.5f) acc += 1.0f;
        }
        total_test_loss += loss / seq_len;
        total_test_acc += acc / seq_len;
    }
    float avg_test_loss = total_test_loss / test_batch.size();
    float avg_test_acc = total_test_acc / test_batch.size() * 100;
    std::cout << "测试集 Loss: " << std::fixed << std::setprecision(4) << avg_test_loss
              << " | Acc: " << std::fixed << std::setprecision(2) << avg_test_acc << "%" << std::endl;

    // 6. 训练完成后显示注意力热力图
    Tensor3D finalAttn = m_transformer->getLastLayerAttentionWeights();
    plotAttentionHeatmap(finalAttn);
    std::cout << "\n训练完成，已显示注意力热力图" << std::endl;

    // ======================
    // 修复3：训练结束后最终刷新横轴范围
    // ======================
    m_lossSeries->clear();
    for (int i = 0; i < m_lossHistory.size(); ++i) {
        m_lossSeries->append(i, m_lossHistory[i]);
    }
    m_accSeries->clear();
    for (int i = 0; i < m_accHistory.size(); ++i) {
        m_accSeries->append(i, m_accHistory[i]);
    }
    // 强制设置横轴为总训练轮数
    static_cast<QValueAxis*>(m_lossChart->axes(Qt::Horizontal).first())->setRange(0, epochs);
    static_cast<QValueAxis*>(m_accChart->axes(Qt::Horizontal).first())->setRange(0, epochs);
    // 自适应Loss纵轴最终范围
    float final_max_loss = *std::max_element(m_lossHistory.begin(), m_lossHistory.end());
    static_cast<QValueAxis*>(m_lossChart->axes(Qt::Vertical).first())->setRange(0, final_max_loss * 1.1);

    m_lossChart->createDefaultAxes();
    m_accChart->createDefaultAxes();
    m_lossView->repaint();
    m_accView->repaint();

    m_trainBtn->setEnabled(true);
}
