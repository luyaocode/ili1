#include "guimanager.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QCalendarWidget>
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QDate>
#include <QTextCharFormat>
#include <QFont>
#include <QDebug>
#include <QPushButton>
#include "configparser.h"
#include "datamanager.h"
#include "commontool.h"

GuiManager::GuiManager(QObject *parent): QObject(parent)
{
}

GuiManager::~GuiManager()
{
}

GuiManager& GuiManager::getInst()
{
    static GuiManager inst;
    return inst;
}

void GuiManager::showCalendar(const QList<QDate> &highlightDates)
{
    m_window = createMainWindow(highlightDates);
    m_window->show();
}

void GuiManager::onCalendarPageChanged(int year, int month)
{
    DataManager datamgr;
    auto        list      = datamgr.getUsageDates(year, month);
    QString     statsText = QString("%1月总计：%2 天").arg(month).arg(list.size());
    m_statsLabel->setText(statsText);
}

QWidget *GuiManager::createMainWindow(const QList<QDate> &highlightDates)
{
    m_markedDates   = highlightDates;
    QWidget *window = new QWidget();
    window->setWindowTitle("电脑使用记录");

    QVBoxLayout *layout = new QVBoxLayout(window);

    // 添加说明标签
    QHBoxLayout *hlayout = new QHBoxLayout();
    QLabel *label_time_1 = new QLabel("工作日：" + formatTime(ConfigParser::getInst().getConfig().timeRanges[0].first) +
                                          "至" + formatTime(ConfigParser::getInst().getConfig().timeRanges[0].second),
                                      window);
    hlayout->addWidget(label_time_1);
    QLabel *label_time_2 = new QLabel("周末：" + formatTime(ConfigParser::getInst().getConfig().timeRanges[1].first) +
                                          "至" + formatTime(ConfigParser::getInst().getConfig().timeRanges[1].second),
                                      window);
    hlayout->addWidget(label_time_2);
    layout->addLayout(hlayout);
    // 创建日历控件
    QCalendarWidget *calendar = new QCalendarWidget(window);
    connect(calendar, &QCalendarWidget::currentPageChanged, this, &GuiManager::onCalendarPageChanged);
    // 创建"回到今天"按钮
    QPushButton *todayButton = new QPushButton("回到今天");

    // 连接按钮点击信号到槽函数
    connect(todayButton, &QPushButton::clicked, [calendar]() {
        // 切换到当前日期所在的页面
        QDate currentDate = QDate::currentDate();
        calendar->setCurrentPage(currentDate.year(), currentDate.month());
        // 选中当前日期
        calendar->setSelectedDate(currentDate);
    });
    hlayout->addStretch();  // 占位，将按钮推到右侧
    hlayout->addWidget(todayButton);

    // 设置当前月份
    QDate currentDate = QDate::currentDate();
    calendar->setCurrentPage(currentDate.year(), currentDate.month());

    // 1. 创建普通高亮日期的格式（红色背景+白色文字）
    QTextCharFormat highlightFormat;
    highlightFormat.setForeground(Qt::white);
    highlightFormat.setBackground(QColor("#FF6666"));
    QFont font = highlightFormat.font();
    font.setBold(true);
    highlightFormat.setFont(font);

    for (const QDate &date : highlightDates)
    {
        calendar->setDateTextFormat(date, highlightFormat);
    }
    layout->addWidget(calendar);

    // 添加统计信息
    QList<QDate> currentMonthDates;
    int          currentYear  = currentDate.year();
    int          currentMonth = currentDate.month();
    foreach (const QDate &date, highlightDates)
    {
        // 检查日期是否有效，并且属于当前年份和月份
        if (date.isValid() && date.year() == currentYear && date.month() == currentMonth)
        {
            currentMonthDates.append(date);
        }
    }
    // 构建包含HTML的统计文本，让数字部分更大更粗
    QString statsText =
        QString("%1月总计：<span style='font-size: 16pt; font-weight: bold; color: #2c3e50;'>%2</span> 天")
            .arg(currentMonth)
            .arg(currentMonthDates.size());

    // 创建标签并设置支持HTML格式
    m_statsLabel = new QLabel(statsText, window);
    m_statsLabel->setTextFormat(Qt::RichText);  // 启用富文本支持

    // 可以额外设置整个标签的基础字体
    QFont baseFont = m_statsLabel->font();
    baseFont.setPointSize(10);  // 设置基础文字大小
    m_statsLabel->setFont(baseFont);

    layout->addWidget(m_statsLabel);
    window->adjustSize();
    Commontool::moveToBottomLeft(window);
    return window;
}

QString GuiManager::formatTime(double time)
{
    if (time < 0)
    {
        return "0:00";  // 处理无效时间
    }

    // 提取小时部分（整数部分）
    int hours = static_cast<int>(time);

    // 提取分钟部分（小数部分转换为分钟）
    double minutesDecimal = time - hours;
    int    minutes        = static_cast<int>(minutesDecimal * 60 + 0.5);  // +0.5用于四舍五入

    // 处理分钟可能超过59的情况（如8.999小时约等于8小时59分56秒）
    if (minutes >= 60)
    {
        minutes -= 60;
        hours += 1;
    }

    // 格式化输出，确保分钟为两位数
    return QString("%1:%2").arg(hours).arg(minutes, 2, 10, QChar('0'));
}
