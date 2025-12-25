#include "MainWindow.h"
#include <QVBoxLayout>
#include <QFileInfo>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    initUi();
    initConnections();
    setWindowTitle("XML浏览器");
    resize(1200, 800);
}

void MainWindow::initUi()
{
    // 主分割器（左右分栏）
    m_splitter = new QSplitter(Qt::Horizontal, this);

    // 左侧文件浏览器
    m_fileExplorer = new FileExplorer(this);
    m_splitter->addWidget(m_fileExplorer);

    // 右侧主容器（用于放置垂直分割器）
    QWidget *rightContainer = new QWidget(this);
    m_splitter->addWidget(rightContainer);

    // 右侧垂直分割器（上下分栏）
    QSplitter *rightSplitter = new QSplitter(Qt::Vertical, rightContainer);

    // 上部分：内容堆叠窗口（切换XML/文本视图）
    m_contentStack = new QStackedWidget(rightSplitter);
    m_xmlViewer = new XmlViewer(this);
    m_textViewer = new TextViewer(this);
    m_contentStack->addWidget(m_xmlViewer);
    m_contentStack->addWidget(m_textViewer);
    rightSplitter->addWidget(m_contentStack);

    // 下部分：表格控件
    m_attrViewer = new AttrViewer(rightSplitter);
    rightSplitter->addWidget(m_attrViewer);

    // 设置右侧垂直分割器的初始比例（上70%，下30%）
    rightSplitter->setSizes({700, 300});

    // 右侧容器使用布局管理垂直分割器
    QVBoxLayout *rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0); // 去除边距
    rightLayout->addWidget(rightSplitter);

    // 设置主分割器初始比例（左25%，右75%）
    m_splitter->setSizes({300, 900});

    // 创建主布局，用于放置工具栏和主分割器
    QVBoxLayout *mainLayout = new QVBoxLayout();
    mainLayout->setContentsMargins(0, 0, 0, 0); // 去除布局边距
    mainLayout->setSpacing(0); // 去除控件间距


    // 工具栏
    m_toolBar = new TopButtonToolBar(this);
    m_toolBar->initControls();
    m_toolBar->addButton(QString("打开目录"),std::bind(&FileExplorer::openSourceDir,m_fileExplorer));
    // 先添加工具栏（会自动占满一行宽度）
    mainLayout->addWidget(m_toolBar);
    // 再添加主分割器（会占据剩余空间）
    mainLayout->addWidget(m_splitter);

    // 创建一个主容器部件，设置主布局
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    // 将主容器设置为主窗口的中心部件
    setCentralWidget(centralWidget);
}

void MainWindow::initConnections()
{
    // 连接文件选择信号
    connect(m_fileExplorer, &FileExplorer::fileSelected,
            this, &MainWindow::onFileSelected);
    connect(m_xmlViewer, &XmlViewer::itemSelected,
            m_attrViewer, &AttrViewer::slotData);
    connect(m_toolBar, &TopButtonToolBar::sigExit,
            this, [this](){
        close();
    });
}

void MainWindow::onFileSelected(const QString& filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) return;

    // 根据文件后缀选择视图
    if (fileInfo.suffix().toLower() == "xml")
    {
        m_xmlViewer->loadXmlFile(filePath);
        m_contentStack->setCurrentWidget(m_xmlViewer);
    }
    else
    {
        m_textViewer->loadTextFile(filePath);
        m_contentStack->setCurrentWidget(m_textViewer);
    }
}
