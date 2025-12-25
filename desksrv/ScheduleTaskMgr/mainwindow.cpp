#include "mainwindow.h"
#include <QFile>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonArray>
#include <QJsonObject>
#include "commontool/commontool.h"
#include "scheduleitem.h"
#include "schedulemodel.h"
#include "tool.h"
#include "def.h"

MainWindow::MainWindow(const QList<ScheduleItem> &schedules, QWidget *parent): QMainWindow(parent)
{
    initUi();
    m_scheduleModel->loadConfig(schedules);
}
MainWindow::~MainWindow()
{
}

ScheduleModel *MainWindow::getScheduleModel() const
{
    return m_scheduleModel;
}

void MainWindow::initUi()
{
    m_scheduleModel = new ScheduleModel(this);

    m_scheduleView = new QTableView(this);
    m_scheduleView->setMouseTracking(true);  // 必须开启，否则无法捕获MouseMove
    m_scheduleView->setModel(m_scheduleModel);
    m_scheduleView->setShowGrid(true);
    m_scheduleView->setGridStyle(Qt::DotLine);
    m_scheduleView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_scheduleView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_scheduleView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_scheduleView->setSelectionMode(QAbstractItemView::SingleSelection);

    // 3. 配置表头
    QHeaderView *horizontalHeader = m_scheduleView->horizontalHeader();
    horizontalHeader->setStretchLastSection(true);
    horizontalHeader->setDefaultAlignment(Qt::AlignCenter);
    horizontalHeader->setFont(QFont("Arial", 10, QFont::Medium));
    horizontalHeader->setSectionResizeMode(QHeaderView::Fixed);  // 列宽固定
    m_scheduleView->verticalHeader()->hide();

    setCentralWidget(m_scheduleView);
    setWindowTitle(tr(APP_NAME));
    resize(800, 800);
}
