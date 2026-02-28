#ifndef TRANSFORMER_DEMO_WINDOW_H
#define TRANSFORMER_DEMO_WINDOW_H

// 必须先包含Qt核心头文件
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QMessageBox>
#include <QLineEdit>
#include <QLabel>
#include <QLineSeries>

// 显式包含Qt Charts所有必要头文件（Qt5.9兼容）
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QAbstractSeries>

// Qt5.9中Qt Charts需要显式使用命名空间
QT_CHARTS_USE_NAMESPACE

#include "transformer_core.h"

// Transformer演示窗口类
class TransformerDemoWindow : public QMainWindow
{
    Q_OBJECT
public:
    // 构造函数
    explicit TransformerDemoWindow(QWidget *parent = nullptr);
    // 析构函数
    ~TransformerDemoWindow() override;

private slots:
    // 功能：运行Transformer演示
    void runDemo();

    // 功能：绘制注意力权重伪热力图（兼容Qt5.9）
    // 参数：attnWeights-三维注意力权重矩阵
    void plotAttentionHeatmap(const Tensor3D &attnWeights);

    void trainModel();  // 新增：训练模型

private:
    // 核心成员变量
    TransformerEncoder *m_transformer;
    QPushButton        *m_runBtn;
    QPushButton        *m_trainBtn;
    QLineEdit          *m_epochEdit;
    QChartView         *m_lossView;     // Loss曲线视图（左）
    QChartView         *m_accView;      // 准确率曲线视图（右）
    QChartView         *m_heatmapView;  // 热力图视图

    // 训练历史数据
    std::vector<float> m_lossHistory;
    std::vector<float> m_accHistory;

    // 图表相关对象
    QLineSeries *m_lossSeries = nullptr;
    QLineSeries *m_accSeries  = nullptr;
    QChart      *m_lossChart  = nullptr;
    QChart      *m_accChart   = nullptr;
};

#endif  // TRANSFORMER_DEMO_WINDOW_H
