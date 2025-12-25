#include "mainwindow.h"
#include <QHeaderView>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QFont>
#include <QWidget>
#include <QProcess>
#include <QMenuBar>
#include <QFileDialog>

#include "tool.h"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent)
{
    m_mainTabWidget = new QTabWidget(this);
    m_mainTabWidget->setTabPosition(QTabWidget::North);
    initToolTabPage();
    initCommonTabPage();
    loadConfig();
    setCentralWidget(m_mainTabWidget);
    setWindowTitle(tr("QuickManager"));
    resize(640, 800);
    // 菜单
    initMenubar();

    connect(m_toolModel, &ToolModel::sigSaveConfig, this, &MainWindow::slotSaveConfig);
    connect(m_commonModel, &CommonModel::sigSaveConfig, this, &MainWindow::slotSaveConfig);
    connect(m_commonDelegate, &CommonDelegate::btnClicked, this, [this](int row, CommonDelegate::ButtonType type) {
        auto path = m_commonModel->data(m_commonModel->index(row, 1)).toString();
        if (type == CommonDelegate::ButtonType::TerminalBtn)
        {
            emit sigOpenTerminal(path);
        }
        else if (type == CommonDelegate::ButtonType::DirectoryBtn)
        {
            emit sigOpenDirectory(path);
        }
    });
}

void MainWindow::initToolTabPage()
{
    // 1. 创建Tab页容器
    QWidget     *toolTabPage = new QWidget(this);
    QVBoxLayout *toolLayout  = new QVBoxLayout(toolTabPage);
    toolLayout->setContentsMargins(10, 10, 10, 10);

    // 2. 初始化表格（复用原有逻辑）
    m_toolModel    = new ToolModel(this);
    m_toolDelegate = new ToolDelegate(this);

    m_toolTableView = new QTableView(this);
    m_toolTableView->setModel(m_toolModel);
    m_toolTableView->setItemDelegate(m_toolDelegate);
    m_toolTableView->setShowGrid(true);
    m_toolTableView->setGridStyle(Qt::DotLine);
    m_toolTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_toolTableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_toolTableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_toolTableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // 3. 配置表头
    QHeaderView *horizontalHeader = m_toolTableView->horizontalHeader();
    horizontalHeader->setStretchLastSection(true);
    horizontalHeader->setDefaultAlignment(Qt::AlignCenter);
    horizontalHeader->setFont(QFont("Arial", 10, QFont::Medium));
    // 1. 取消全局Stretch模式
    horizontalHeader->setSectionResizeMode(QHeaderView::Fixed);  // 列宽固定
    // 2. 为指定列设置具体宽度
    horizontalHeader->resizeSection(ToolModel::Columns::NameColumn, 120);
    horizontalHeader->resizeSection(ToolModel::Columns::PathColumn, 160);
    horizontalHeader->resizeSection(ToolModel::Columns::StatusColumn, 80);
    // 3. 最后一列设置为拉伸模式
    horizontalHeader->setSectionResizeMode(ToolModel::Columns::ShortcutColumn, QHeaderView::Stretch);
    m_toolTableView->verticalHeader()->hide();

    // 5. 添加到布局
    toolLayout->addWidget(m_toolTableView);

    // 6. 将页面添加到主TabWidget
    m_mainTabWidget->addTab(toolTabPage, tr("快捷工具"));
}

void MainWindow::initCommonTabPage()
{
    // 1. 创建Tab页容器
    QWidget     *commonTabPage = new QWidget(this);
    QVBoxLayout *commonLayout  = new QVBoxLayout(commonTabPage);
    commonLayout->setContentsMargins(10, 10, 10, 10);

    // 2. 初始化表格（复用原有逻辑）
    m_commonModel    = new CommonModel(this);
    m_commonDelegate = new CommonDelegate(this);

    m_commonTableView = new QTableView(this);
    m_commonTableView->setMouseTracking(true);  // 必须开启，否则无法捕获MouseMove
    m_commonTableView->setModel(m_commonModel);
    m_commonTableView->setItemDelegate(m_commonDelegate);
    m_commonTableView->setShowGrid(true);
    m_commonTableView->setGridStyle(Qt::DotLine);
    m_commonTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_commonTableView->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    m_commonTableView->setSelectionBehavior(QAbstractItemView::SelectItems);
    m_commonTableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // 3. 配置表头
    QHeaderView *horizontalHeader = m_commonTableView->horizontalHeader();
    horizontalHeader->setStretchLastSection(true);
    horizontalHeader->setDefaultAlignment(Qt::AlignCenter);
    horizontalHeader->setFont(QFont("Arial", 10, QFont::Medium));
    // 1. 取消全局Stretch模式
    horizontalHeader->setSectionResizeMode(QHeaderView::Fixed);  // 列宽固定
    // 2. 为指定列设置具体宽度
    horizontalHeader->resizeSection(CommonModel::Columns::NameColumn, 110);
    horizontalHeader->resizeSection(CommonModel::Columns::PathColumn, 150);
    horizontalHeader->resizeSection(CommonModel::Columns::StatusColumn, 80);
    horizontalHeader->resizeSection(CommonModel::Columns::OperationColunm, 160);
    // 3. 最后一列设置为拉伸模式
    horizontalHeader->setSectionResizeMode(CommonModel::Columns::ShortcutColumn, QHeaderView::Stretch);
    m_commonTableView->verticalHeader()->hide();

    // 5. 添加到布局
    commonLayout->addWidget(m_commonTableView);

    // 6. 将页面添加到主TabWidget
    m_mainTabWidget->addTab(commonTabPage, tr("快捷目录"));
}

void MainWindow::loadConfig()
{
    const QString cfgName = "config.json";
    // Qt5.9 直接使用 QByteArray 解析 JSON
    QString realFilePath = Commontool::getConfigFilePath("QuickManager", cfgName);
    QFile   file(realFilePath);

    // 若文件不存在，创建空配置文件（避免首次运行解析失败）
    if (!file.exists())
    {
        qInfo() << "Config file not found: " << realFilePath;
        createConfigFile(realFilePath);
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open config file:" << realFilePath;
        return;
    }
    QByteArray jsonData = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument   doc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError)
    {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return;
    }

    if (!doc.isArray())
    {
        qWarning() << "Invalid config file format (not array)";
        return;
    }
    QList<QuickItem> listTool;
    QList<QuickItem> listCommon;
    QJsonArray       array = doc.array();
    for (const QJsonValue &val : array)
    {
        if (val.isObject())
        {
            QuickItem item = QuickItem::fromVariantMap(val.toObject().toVariantMap());
            if (item.type == QuickType::Tool)
            {
                listTool.append(item);
            }
            else if (item.type == QuickType::Common)
            {
                listCommon.append(item);
            }
        }
    }
    m_toolModel->loadConfig(listTool);
    m_commonModel->loadConfig(listCommon);
}

void MainWindow::onRestoreDefault()
{
    const QString cfgName      = "config.json";
    QString       realFilePath = Commontool::getConfigFilePath("QuickManager", cfgName);
    QFile         configFile(realFilePath);
    if (configFile.exists())
    {
        bool deleteSuccess = configFile.remove();
        // 4. 处理删除结果
        if (deleteSuccess)
        {
            qDebug() << "配置文件删除成功：" << realFilePath;
        }
    }
    loadConfig();
}

void MainWindow::slotSaveConfig()
{
    const QList<QuickItem> tools  = m_toolModel->getData();
    const QList<QuickItem> common = m_commonModel->getData();
    QJsonArray             array;
    for (const QuickItem &item : tools)
    {
        array.append(QJsonObject::fromVariantMap(item.toVariantMap()));
    }
    for (const QuickItem &item : common)
    {
        array.append(QJsonObject::fromVariantMap(item.toVariantMap()));
    }
    const QString cfgName      = "config.json";
    QString       realFilePath = Commontool::getConfigFilePath("QuickManager", cfgName);
    QFile         file(realFilePath);

    if (!file.exists())
    {
        qInfo() << "Config file not found, creating new one:" << realFilePath;
        createConfigFile(realFilePath);
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Failed to save config file:" << cfgName;
        return;
    }

    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.close();
}

MainWindow::~MainWindow()
{
}

ToolModel *MainWindow::getToolModel() const
{
    return m_toolModel;
}

CommonModel *MainWindow::getCommonModel() const
{
    return m_commonModel;
}

void MainWindow::initMenubar()
{
    QMenuBar *menuBar              = this->menuBar();
    QMenu    *settingMenu          = menuBar->addMenu(tr("设置"));
    QAction  *restoreDefaultAction = new QAction(tr("恢复默认设置"), this);
    settingMenu->addAction(restoreDefaultAction);
    connect(restoreDefaultAction, &QAction::triggered, this, &MainWindow::onRestoreDefault);
}
