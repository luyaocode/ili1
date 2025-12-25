// MainWindow.cpp（更新后）
#include "MainWindow.h"
#include <QMessageBox>
#include <QDir>
#include <QGridLayout>
#include <QGroupBox>
#include <QDebug>

// 全局变量：暂存当前 MainWindow 实例（仅用于编码时传递 this）
static MainWindow *g_current_main_window = nullptr;

// 全局辅助函数：包装 generateFilteredFrame，适配普通函数指针
static QImage global_frame_generator(int frame_index, int width, int height)
{
    Q_UNUSED(width)
    Q_UNUSED(height)
    if (g_current_main_window)
    {
        // 调用 MainWindow 的成员函数，width/height 已在 MainWindow 中存储，无需传递
        return g_current_main_window->generateFilteredFrame(frame_index);
    }
    return QImage();  // 异常情况返回空图像
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent)
{
    this->setWindowTitle("VideoGenerator");
    this->resize(512, 640);

    // ---------------------- UI 布局 ----------------------
    QWidget *centralWidget = new QWidget(this);
    this->setCentralWidget(centralWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // 1. 生成参数配置区（网格布局）
    QGroupBox   *paramGroup  = new QGroupBox("生成参数");
    QGridLayout *paramLayout = new QGridLayout(paramGroup);

    // 参数标签+输入框
    paramLayout->addWidget(new QLabel("分辨率（宽×高）："), 0, 0);
    m_widthSpin = new QSpinBox;
    m_widthSpin->setRange(320, 1920);
    m_widthSpin->setValue(640);
    m_heightSpin = new QSpinBox;
    m_heightSpin->setRange(240, 1080);
    m_heightSpin->setValue(480);
    paramLayout->addWidget(m_widthSpin, 0, 1);
    paramLayout->addWidget(new QLabel("×"), 0, 2);
    paramLayout->addWidget(m_heightSpin, 0, 3);

    paramLayout->addWidget(new QLabel("帧率（fps）："), 1, 0);
    m_fpsSpin = new QSpinBox;
    m_fpsSpin->setRange(1, 60);
    m_fpsSpin->setValue(30);
    paramLayout->addWidget(m_fpsSpin, 1, 1, 1, 3);

    paramLayout->addWidget(new QLabel("时长（秒）："), 2, 0);
    m_durationSpin = new QSpinBox;
    m_durationSpin->setRange(1, 60);
    m_durationSpin->setValue(5);
    paramLayout->addWidget(m_durationSpin, 2, 1, 1, 3);

    paramLayout->addWidget(new QLabel("输出路径："), 3, 0);
    m_outputEdit           = new QLineEdit("./output.mp4");
    QPushButton *browseBtn = new QPushButton("浏览");
    paramLayout->addWidget(m_outputEdit, 3, 1, 1, 2);
    paramLayout->addWidget(browseBtn, 3, 3);

    // 2. 功能按钮区
    QHBoxLayout *btnLayout           = new QHBoxLayout;
    QPushButton *exportBtn           = new QPushButton("导出视频");
    QPushButton *hotUpdateBtn        = new QPushButton("热更选中插件");
    QPushButton *hotUpdatePluginsBtn = new QPushButton("更新插件列表");
    btnLayout->addWidget(exportBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(hotUpdateBtn);
    btnLayout->addWidget(hotUpdatePluginsBtn);

    // 3. 插件列表区（左右分栏）
    QHBoxLayout *pluginLayout = new QHBoxLayout;
    m_generatorList           = new QListWidget;
    m_generatorList->setToolTip("视频生成器");
    m_filterList = new QListWidget;
    m_filterList->setToolTip("滤镜效果");
    // 无滤镜
    m_filterList->addItem("无");
    m_plugins.push_back(Plugin(PluginInfo(), std::move(DynamicLibrary())));

    pluginLayout->addWidget(m_generatorList);
    pluginLayout->addWidget(m_filterList);

    // 4. 预览区
    m_previewLabel = new QLabel("点击「预览帧」查看效果");
    m_previewLabel->setFixedSize(256, 256);  // 固定预览区尺寸（根据需求调整）
    // 或使用尺寸策略（推荐，更灵活）
    m_previewLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_previewLabel->setMinimumSize(256, 256);  // 最小尺寸
    m_previewLabel->setMaximumSize(256, 256);  // 最大尺寸
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("border: 1px solid #ccc;");

    // 组装布局
    mainLayout->addWidget(paramGroup);
    mainLayout->addLayout(btnLayout);
    mainLayout->addLayout(pluginLayout);
    mainLayout->addWidget(m_previewLabel, 1);  // 占满剩余空间

    // ---------------------- 信号槽连接 ----------------------
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::onExportVideo);
    connect(m_generatorList, &QListWidget::currentRowChanged, this, &MainWindow::onSwitchGenerator);
    connect(m_filterList, &QListWidget::currentRowChanged, this, &MainWindow::onSwitchFilter);
    connect(hotUpdateBtn, &QPushButton::clicked, this, &MainWindow::onHotUpdatePlugin);
    connect(hotUpdatePluginsBtn, &QPushButton::clicked, this, &MainWindow::onHotUpdatePlugins);
    connect(browseBtn, &QPushButton::clicked, [this]() {
        QString filePath = QFileDialog::getSaveFileName(this, "选择输出文件", ".", "视频文件 (*.mp4 *.avi *.gif)");
        if (!filePath.isEmpty())
        {
            m_outputEdit->setText(filePath);
        }
    });

    // ---------------------- 初始化加载插件 ----------------------
    scanPlugins("./plugins/generator", m_generatorList, m_plugins);
    scanPlugins("./plugins/filter", m_filterList, m_plugins);
}

MainWindow::~MainWindow()
{
}

// 扫描插件目录（逻辑不变，仅适配生成器插件）
void MainWindow::scanPlugins(const std::string &plugin_dir, QListWidget *list_widget, std::vector<Plugin> &plugins)
{
    QDir dir(QString::fromStdString(plugin_dir));
    if (!dir.exists())
    {
        QMessageBox::warning(this, "警告", "插件目录不存在：" + dir.path());
        return;
    }

    QStringList filters;
    filters << "*.so";
    dir.setNameFilters(filters);
    QFileInfoList fileList = dir.entryInfoList();

    std::vector<std::string> failedPluginPaths;
    for (const QFileInfo &file : fileList)
    {
        std::string    path = file.absoluteFilePath().toStdString();
        DynamicLibrary lib;
        if (!lib.load(path))
        {
            failedPluginPaths.push_back(path);
            continue;
        }

        auto       get_plugin_info = lib.get_function<PluginInfoFunc>("get_plugin_info");
        PluginInfo info            = get_plugin_info();
        // 判断是生成器插件还是滤镜插件
        QListWidgetItem* item =new QListWidgetItem;
        if (info.type == PluginType::Generator)
        {
            item->setText(QString("%1（支持：.%2）")
                             .arg(QString::fromStdString(info.name))
                             .arg(QString::fromStdString(info.extra)));
            list_widget->addItem(item);
        }
        else if (info.type == PluginType::Filter)
        {
            item->setText(QString::fromStdString(info.name));
            list_widget->addItem(item);
        }
        else
        {
            continue;
        }
        plugins.push_back(Plugin(info, std::move(lib)));
    }
    if (!failedPluginPaths.empty())
    {
        std::string info;
        for (auto path : failedPluginPaths)
        {
            info = info.append(path).append(" ");
        }
        QMessageBox::critical(this, "插件加载失败", QString::fromStdString(info));
    }
}

// 生成单帧并应用滤镜
QImage MainWindow::generateFilteredFrame(int frame_index)
{
    if (!m_currentGenerateFunc)
    {
        throw std::runtime_error("未选中视频生成器");
    }

    int width  = m_widthSpin->value();
    int height = m_heightSpin->value();

    // 1. 生成原始帧
    QImage frame = m_currentGenerateFunc(frame_index, width, height);
    if (frame.isNull())
    {
        throw std::runtime_error("帧生成失败");
    }

    // 2. 应用滤镜（如果选中）
    if (m_currentFilterFunc)
    {
        frame = m_currentFilterFunc(frame);
    }

    return frame;
}

// 预览帧（显示第10帧效果）
void MainWindow::onPreviewFrame()
{
    try
    {
        std::lock_guard<std::mutex> lock(m_pluginMutex);
        QImage                      frame = generateFilteredFrame(10);  // 预览第10帧
        if (frame.isNull())
        {
            m_previewLabel->setText(QString("预览失败：图像为空"));
            return;
        }
        // 缩放适配预览区
        QSize  targetSize  = m_previewLabel->minimumSize();  // 或直接用固定值QSize(640, 480)
        QImage scaledFrame = frame.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_previewLabel->setPixmap(QPixmap::fromImage(scaledFrame));
    }
    catch (const std::runtime_error &e)
    {
        m_previewLabel->setText(QString("预览失败：") + e.what());
    }
}

// 导出视频（核心逻辑：帧生成→滤镜→编码）
void MainWindow::onExportVideo()
{
    if (!m_currentGenerateFunc || !m_currentEncodeFunc)
    {
        QMessageBox::warning(this, "警告", "请选中视频生成器和编码器");
        return;
    }

    std::string output_path = m_outputEdit->text().toStdString();
    int         width       = m_widthSpin->value();
    int         height      = m_heightSpin->value();
    int         fps         = m_fpsSpin->value();
    int         duration    = m_durationSpin->value();

    // 弹出进度提示
    QMessageBox::information(
        this, "开始导出",
        QString("正在导出 %1×%2、%3fps、%4秒的视频...").arg(width).arg(height).arg(fps).arg(duration));

    try
    {
        std::lock_guard<std::mutex> lock(m_pluginMutex);
        // 自定义帧生成函数（包装原生成器+滤镜）
        // 关键修改：用全局变量暂存 this，避免 lambda 捕获
        g_current_main_window = this;

        // 直接使用全局辅助函数指针（无状态，可转换为 GenerateFrameFunc）
        bool success = m_currentEncodeFunc(global_frame_generator, width, height, fps, duration, output_path);

        if (success)
        {
            QMessageBox::information(this, "导出成功", "视频已保存至：" + m_outputEdit->text());
        }
        else
        {
            throw std::runtime_error("编码失败");
        }
    }
    catch (const std::runtime_error &e)
    {
        QMessageBox::critical(this, "导出失败", QString::fromStdString(e.what()));
    }
}

// 切换生成器插件
void MainWindow::onSwitchGenerator(int selectedIdx)
{
    if (selectedIdx < 0 || (unsigned int)selectedIdx >= m_plugins.size())
    {
        m_currentGenerateFunc = nullptr;
        m_currentEncodeFunc   = nullptr;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_pluginMutex);
        DynamicLibrary             &lib = m_plugins[selectedIdx].lib;
        // 获取生成器和编码器函数
        m_currentGenerateFunc = lib.get_function<GenerateFrameFunc>("create_frame_generator");
        m_currentEncodeFunc   = lib.get_function<EncodeVideoFunc>("create_video_encoder");
    }
    onPreviewFrame();  // 切换后更新预览
}

// 切换滤镜插件（逻辑不变）
void MainWindow::onSwitchFilter(int selectedIdx)
{
    if (selectedIdx < 0 || (unsigned int)selectedIdx >= m_plugins.size())
    {
        m_currentFilterFunc = nullptr;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(m_pluginMutex);
        DynamicLibrary             &lib = m_plugins[selectedIdx].lib;
        m_currentFilterFunc             = lib.get_function<FilterFunc>("create_filter");
    }

    onPreviewFrame();  // 切换后更新预览
}

// 热更选中插件（逻辑不变，适配生成器）
void MainWindow::onHotUpdatePlugin()
{
    if (m_generatorList->currentRow() >= 0)
    {
        int         idx      = m_generatorList->currentRow();
        std::string lib_path = m_plugins[idx].lib.get_path();
        try
        {
            std::lock_guard<std::mutex> lock(m_pluginMutex);
            m_plugins[idx].lib.load(lib_path);
            m_currentGenerateFunc = m_plugins[idx].lib.get_function<GenerateFrameFunc>("create_frame_generator");
            m_currentEncodeFunc   = m_plugins[idx].lib.get_function<EncodeVideoFunc>("create_video_encoder");
            QMessageBox::information(this, "热更成功",
                                     "生成器插件 " + m_generatorList->currentItem()->text() + " 已更新");
        }
        catch (const std::runtime_error &e)
        {
            QMessageBox::critical(this, "热更失败", QString::fromStdString(e.what()));
        }
    }
    if (m_filterList->currentRow() >= 0)
    {
        int         idx      = m_filterList->currentRow();
        std::string lib_path = m_plugins[idx].lib.get_path();
        if (!lib_path.empty())
        {
            try
            {
                std::lock_guard<std::mutex> lock(m_pluginMutex);
                m_plugins[idx].lib.load(lib_path);
                m_currentFilterFunc = m_plugins[idx].lib.get_function<FilterFunc>("create_filter");
                QMessageBox::information(this, "热更成功",
                                         "滤镜插件 " + m_filterList->currentItem()->text() + " 已更新");
            }
            catch (const std::runtime_error &e)
            {
                QMessageBox::critical(this, "热更失败", QString::fromStdString(e.what()));
            }
        }
    }
    onPreviewFrame();
}

void MainWindow::onHotUpdatePlugins()
{
    {
        std::lock_guard<std::mutex> lock(m_pluginMutex);
        // ---------------------- 更新插件列表 ----------------------
        m_generatorList->clear();
        m_filterList->clear();
        for (auto &plugin : m_plugins)
        {
            plugin.lib.unload();
        }
        scanPlugins("./plugins/generator", m_generatorList, m_plugins);
        scanPlugins("./plugins/filter", m_filterList, m_plugins);
    }
    onPreviewFrame();
}
