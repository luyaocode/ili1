#ifndef CHARTDIALOG_H
#define CHARTDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QChart>
#include <QLineSeries>
#include <QChartView>
#include <QDateTimeAxis>
#include <QValueAxis>
#include <QPainter>
#include <QPixmap>
#include <QDateTime>
#include <QRegExp>
#include <QFile>
#include <QTextStream>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

QT_CHARTS_USE_NAMESPACE

// 日志数据结构体
struct LogData {
    QDateTime time;       // 日志时间
    qint64 elapsedMs;     // startScan耗时(ms)
};

class ChartDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChartDialog(QWidget *parent = nullptr);
    ~ChartDialog() override;

private slots:
    // 选择日志文件并解析绘图
    void onSelectLogFile();
    // 保存图表到本地
    void onSaveChart();

private:
    // 解析日志文件
    QVector<LogData> parseLogFile(const QString &filePath);
    // 初始化图表控件
    void initChart();

private:
    QChart          *m_chart        = nullptr;
    QLineSeries     *m_series       = nullptr;
    QChartView      *m_chartView    = nullptr;
    QPushButton     *m_btnSelectLog = nullptr;
    QPushButton     *m_btnSaveChart = nullptr;
    QVector<LogData> m_logData;  // 存储解析后的日志数据
};

#endif  // CHARTDIALOG_H
