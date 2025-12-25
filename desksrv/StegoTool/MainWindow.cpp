#include "MainWindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include "tool.h"
#include "commontool/commontool.h"
#include "unify/signaldebugger.hpp"

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("StegoTool");
    setAcceptDrops(true);

    // 初始化界面样式
    ui->bitCountCombo->addItem("1位LSB（隐蔽性最高）");
    ui->bitCountCombo->addItem("2位LSB（容量更大）");
    ui->bitCountCombo->addItem("3位LSB");
    ui->bitCountCombo->addItem("4位LSB");
    ui->bitCountCombo->addItem("5位LSB");
    ui->bitCountCombo->addItem("6位LSB");
    ui->bitCountCombo->addItem("7位LSB");
    ui->bitCountCombo->addItem("8位LSB（图片完全损坏）");
    ui->bitCountCombo->setCurrentIndex(0);

    initGenImageSizeComboBox();

    // 预览标签样式
    ui->originalPreview->setStyleSheet("border: 2px dashed #cccccc; border-radius: 4px;");
    ui->processedPreview->setStyleSheet("border: 2px dashed #cccccc; border-radius: 4px;");
    ui->originalPreview->setScaledContents(true);
    ui->processedPreview->setScaledContents(true);

    // 文本框样式
    ui->textEdit->setStyleSheet("QTextEdit { border: 1px solid #cccccc; border-radius: 4px; padding: 5px; }");

    // 按钮样式
    QString btnStyle = "QPushButton { background-color: #2196F3; color: white; border: none; "
                       "border-radius: 4px; padding: 6px 12px; }"
                       "QPushButton:hover { background-color: #1976D2; }"
                       "QPushButton:disabled { background-color: #BBDEFB; }";
    ui->btnOpenImage->setStyleSheet(btnStyle);
    ui->btnGenImage->setStyleSheet(btnStyle);
    ui->btnEmbed->setStyleSheet(btnStyle);
    ui->btnExtract->setStyleSheet(btnStyle);
    ui->btnSaveImage->setStyleSheet(btnStyle);
    ui->btnEmbedBinary->setStyleSheet(btnStyle);
    ui->btnExtractBinary->setStyleSheet(btnStyle);

    // 禁用初始状态的按钮
    ui->btnEmbed->setDisabled(true);
    ui->btnExtract->setDisabled(true);
    ui->btnEmbedBinary->setDisabled(true);
    ui->btnExtractBinary->setDisabled(true);
    ui->btnSaveImage->setDisabled(true);

    // 连接信号槽
    connect(ui->textEdit, &QTextEdit::textChanged, this, &MainWindow::updateEmbedInfo);
    connect(this, &MainWindow::sigImageCreated, this, &MainWindow::slotImageCreated);
    connect(this, &MainWindow::sigEmbedStart, this, &MainWindow::slotEmbedStart);
    connect(this, &MainWindow::sigEmbedProgress, this, &MainWindow::slotEmbedProgress);
    connect(this, &MainWindow::sigEmbedCompleted, this, &MainWindow::slotEmbedCompleted);
    connect(this, &MainWindow::sigExtractStart, this, &MainWindow::slotExtractStart);
    connect(this, &MainWindow::sigExtractProgress, this, &MainWindow::slotExtractProgress);
    connect(this, &MainWindow::sigExtractCompleted, this, &MainWindow::slotExtractCompleted);
    connect(this, &MainWindow::sigImageSaveStart, this, &MainWindow::slotImageSaveStart);
    connect(this, &MainWindow::sigImageSaved, this, &MainWindow::slotImageSaved);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() == 1)
    {
        QString filePath = event->mimeData()->urls().first().toLocalFile();
        QString suffix   = QFileInfo(filePath).suffix().toLower();
        if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "bmp")
        {
            event->acceptProposedAction();
        }
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    QString filePath = event->mimeData()->urls().first().toLocalFile();
    loadImage(filePath);
}

void MainWindow::loadImage(const QString &filePath)
{
    QImage tmpImage;
    tmpImage.load(filePath);
    loadImage(tmpImage);
}

void MainWindow::loadImage(const QImage &image)
{
    m_originalImage = image;
    if (m_originalImage.isNull())
    {
        showMessage("图片加载失败", true);
        return;
    }
    QString statusInfo = QString("已加载：%1 x %2").arg(m_originalImage.width()).arg(m_originalImage.height());

    QString suffix;
    int len = 0;
    int bitCount = 0;
    auto succ = StegoCore::extractHeader(image,suffix,len,bitCount);
    if(succ)
    {
        statusInfo +=QString(" \n检测到隐藏%1文件，%2，LSB=%3").arg(suffix).arg(Commontool::formatFileSize(len)).arg(bitCount);
    }
    ui->statusLabel->setText(statusInfo);

    showImagePreview(m_originalImage, ui->originalPreview);
    ui->processedPreview->clear();
    m_processedImage = QImage();

    // 启用功能按钮
    ui->btnEmbed->setEnabled(true);
    ui->btnExtract->setEnabled(true);
    ui->btnSaveImage->setDisabled(true);
    ui->btnEmbedBinary->setEnabled(true);
    ui->btnExtractBinary->setEnabled(true);
    updateEmbedInfo();
}

void MainWindow::showImagePreview(QImage &image, QLabel *label)
{
    QSize  labelSize   = label->size();
    QImage scaledImage = image.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    label->setPixmap(QPixmap::fromImage(scaledImage));
}

void MainWindow::showMessage(const QString &text, bool isError)
{
    ui->messageLabel->setText(text);
    ui->messageLabel->setStyleSheet(isError ? "color: #F44336;" : "color: #4CAF50;");
}

void MainWindow::addTaskID(TaskID id)
{
    QMutexLocker lock(&m_mutex);
    m_tasks.append(id);
}

void MainWindow::removeTaskID(TaskID id)
{
    QMutexLocker lock(&m_mutex);
    m_tasks.removeOne(id);
}

void MainWindow::cancelTasks()
{
    QMutexLocker  lock(&m_mutex);
    QList<TaskID> tasksTmp = m_tasks;
    lock.unlock();

    for (auto id : tasksTmp)
    {
        bool succ = AsyncTaskManager::get_instance().cancel_task(id);
        if (succ)
        {
            showMessage("任务已取消");
        }
    }
}

void MainWindow::updateEmbedInfo()
{
    if (m_originalImage.isNull())
        return;

    qint64 maxSize     = StegoCore::getMaxEmbedSize(m_originalImage, m_bitCount);
    qint64 currentSize = ui->textEdit->toPlainText().toUtf8().size();
    ui->embedInfoLabel->setText(
        QString("当前文本大小：%1 / 最大支持：%2").arg(Commontool::formatFileSize(currentSize)).arg(Commontool::formatFileSize(maxSize)));
    // 文本超长时高亮提示
    if (currentSize > maxSize)
    {
        ui->embedInfoLabel->setStyleSheet("color: #F44336;");
    }
    else
    {
        ui->embedInfoLabel->setStyleSheet("color: #666666;");
    }
}

void MainWindow::slotImageCreated()
{
    showMessage("图片创建成功！");
}

void MainWindow::slotEmbedStart()
{
    QMutexLocker lock(&m_mutex);
    m_lastFilter             = createTypeFilter<QLabel, QWidget, MainWindow>(WidgetFilter::Exclude);
    m_lastFilter.namePattern = "btnCancelTask";
    setWidgetEnabledRecursively(this, false, m_lastFilter);
}

void MainWindow::slotEmbedProgress(float pct, const QString &msg)
{
    QString info;
    if (msg.isEmpty())
    {
        info = QString("嵌入进度: %1\%").arg(pct * 100);
    }
    else
    {
        info = QString("嵌入进度: %1\% (%2)").arg(pct * 100).arg(msg);
    }
    showMessage(info);
}

void MainWindow::slotEmbedCompleted(unify::EM_TASK_STATE state, bool result)
{
    PRINT_SIGNAL_DETAILED_SOURCE();
    if (state == EM_TASK_STATE::COMPLETED)
    {
        if (result)
        {
            showMessage(QString("嵌入成功"));
            showImagePreview(m_processedImage, ui->processedPreview);
        }
        else
        {
            showMessage(QString("嵌入失败"), true);
        }
    }
    else
    {
        showMessage(QString("任务执行失败"), true);
    }
    QMutexLocker lock(&m_mutex);
    setWidgetEnabledRecursively(this, true, m_lastFilter);
}

void MainWindow::slotExtractProgress(float pct, const QString &msg)
{
    QString info;
    if (msg.isEmpty())
    {
        info = QString("提取进度: %1\%").arg(pct * 100);
    }
    else
    {
        info = QString("提取进度: %1\% (%2)").arg(pct * 100).arg(msg);
    }
    showMessage(info);
}

void MainWindow::slotExtractStart()
{
    QMutexLocker lock(&m_mutex);
    m_lastFilter             = createTypeFilter<QLabel, QWidget, MainWindow>(WidgetFilter::Exclude);
    m_lastFilter.namePattern = "btnCancelTask";
    setWidgetEnabledRecursively(this, false, m_lastFilter);
}

void MainWindow::slotExtractCompleted(EM_TASK_STATE state, bool result)
{
    if (state == EM_TASK_STATE::COMPLETED)
    {
        if (result)
        {
            showMessage(QString("提取成功"));
        }
        else
        {
            showMessage(QString("提取失败"), true);
        }
    }
    else
    {
        showMessage(QString("任务执行失败"), true);
    }
    QMutexLocker lock(&m_mutex);
    setWidgetEnabledRecursively(this, true, m_lastFilter);
}

void MainWindow::slotImageSaveStart()
{
    showMessage(QString("正在保存图片"));
    QMutexLocker lock(&m_mutex);
    m_lastFilter             = createTypeFilter<QLabel, QWidget, MainWindow>(WidgetFilter::Exclude);
    m_lastFilter.namePattern = "btnCancelTask";
    setWidgetEnabledRecursively(this, false, m_lastFilter);
}

void MainWindow::slotImageSaved(unify::EM_TASK_STATE state, bool result)
{
    if (state == EM_TASK_STATE::COMPLETED)
    {
        showMessage(result ? "图片保存成功！" : "图片保存失败", !result);
    }
    else
    {
        showMessage(QString("任务执行失败"), true);
    }
    QMutexLocker lock(&m_mutex);
    setWidgetEnabledRecursively(this, true, m_lastFilter);
}

void MainWindow::initGenImageSizeComboBox()
{
    // 1. 定义图片尺寸
    QList<QSize> imageSizes = {QSize(512, 512), QSize(1024, 1024), QSize(1920, 1080), QSize(6000, 6000),
                               QSize(20000, 20000)};

    // 2. 清空下拉框
    ui->cmbGenImageSize->clear();

    // 3. 初始化下拉框选项
    ui->cmbGenImageSize->addItem("512×512", QVariant::fromValue(imageSizes[0]));
    ui->cmbGenImageSize->addItem("1024×1024", QVariant::fromValue(imageSizes[1]));
    ui->cmbGenImageSize->addItem("1920x1080", QVariant::fromValue(imageSizes[2]));
    ui->cmbGenImageSize->addItem("6000×6000", QVariant::fromValue(imageSizes[3]));
    ui->cmbGenImageSize->addItem("20000x20000", QVariant::fromValue(imageSizes[4]));
    ui->cmbGenImageSize->setCurrentIndex(0);

    // 4. 绑定选中变化
    connect(ui->cmbGenImageSize, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this,
            [=](int index) {
                saveSelectedImageSize(index);
            });
    saveSelectedImageSize(0);
}

void MainWindow::saveSelectedImageSize(int index)
{
    if (index < 0 || index >= ui->cmbGenImageSize->count())
    {
        return;
    }
    QVariant var = ui->cmbGenImageSize->itemData(index);
    if (!var.canConvert<QSize>())
    {
        return;
    }
    m_genImgSize = var.value<QSize>();
}

void MainWindow::on_btnOpenImage_clicked()
{
    QString filePath =
        QFileDialog::getOpenFileName(this, "选择图片", "", "图片文件 (*.png *.jpg *.jpeg *.bmp);;所有文件 (*.*)");
    if (!filePath.isEmpty())
    {
        loadImage(filePath);
    }
}

void MainWindow::on_btnGenImage_clicked()
{
    auto task_func = [this](const std::function<void(float, float, std::string)> &update_progress,
                            const std::function<bool()>                          &is_cancelled) {
        Q_UNUSED(is_cancelled)
//        auto image = createImage(m_genImgSize.width(), m_genImgSize.height());
        auto image = generateDinosaurImage(m_genImgSize.width(),m_genImgSize.height());
        loadImage(image);
        update_progress(100, 100, "");
    };
    auto process_cb = [this](const TaskProgress<float> &progress) {
        Q_UNUSED(progress)
        emit sigImageCreated();
    };
    TaskID taskId      = 0;
    auto   complete_cb = [this, &taskId](EM_TASK_STATE state) {
        Q_UNUSED(state)
        removeTaskID(taskId);
    };
    taskId = AsyncTaskManager::get_instance().add_task<void>(task_func, nullptr, process_cb, complete_cb);
    addTaskID(taskId);
}

void MainWindow::on_btnEmbed_clicked()
{
    try
    {
        QString text = ui->textEdit->toPlainText();
        if (text.isEmpty())
        {
            showMessage("请输入要嵌入的文本", true);
            return;
        }

        bool success = StegoCore::embedText(m_originalImage, text, m_processedImage, nullptr, nullptr, m_bitCount);
        if (success)
        {
            showImagePreview(m_processedImage, ui->processedPreview);
            showMessage("文本嵌入成功！");
            ui->btnSaveImage->setEnabled(true);
        }
        else
        {
            showMessage("文本嵌入失败", true);
        }
    }
    catch (const std::runtime_error &e)
    {
        showMessage(QString("嵌入失败：%1").arg(e.what()), true);
    }
}

void MainWindow::on_btnExtract_clicked()
{
    QString text;
    try
    {
        bool succ = StegoCore::extractText(m_originalImage, text);
        ui->textEdit->setPlainText(text);
        showMessage(succ ? "文本提取成功！" : "未提取到隐藏文本", !succ);
    }
    catch (const std::runtime_error &e)
    {
        showMessage(QString("提取失败：%1").arg(e.what()), true);
    }
}

void MainWindow::on_btnEmbedBinary_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择文件", "", "所有文件 (*.*);;所有文件 (*.*)");
    if (filePath.isEmpty())
    {
        return;
    }
    auto task_func = [this, filePath](const std::function<void(float, float, std::string)> &update_progress,
                                      const std::function<bool()>                          &is_cancelled) {
        try
        {
            bool success = StegoCore::embedBinary(m_originalImage, filePath, m_processedImage, update_progress,
                                                  is_cancelled, m_bitCount);
            return success;
        }
        catch (const std::runtime_error &e)
        {
            update_progress(0, 0, QString("嵌入文件失败：%1").arg(e.what()).toStdString());
            return false;
        }
    };
    auto start_cb = [this]() {
        emit sigEmbedStart();
    };
    auto process_cb = [this](const TaskProgress<float> &progress) {
        emit sigEmbedProgress(progress.percentage, QString::fromStdString(progress.message));
    };
    TaskID taskId      = 0;
    auto   complete_cb = [this, &taskId](EM_TASK_STATE state, bool result) {
        emit sigEmbedCompleted(state, result);
        removeTaskID(taskId);
    };
    taskId = AsyncTaskManager::get_instance().add_task<bool>(task_func, start_cb, process_cb, complete_cb);
    addTaskID(taskId);
}

void MainWindow::on_btnExtractBinary_clicked()
{
    QString suffix;
    int     len      = 0;
    int     bitCount = 1;
    bool    result   = StegoCore::extractHeader(m_originalImage, suffix, len, bitCount);
    if (!result)
    {
        showMessage("文件头识别错误", true);
        return;
    }

    // 构建默认文件名（包含提取的后缀）
    QString defaultPath = QDir::homePath();

    // 显示保存对话框
    // 选择保存目录（替代原文件保存对话框）
    QString saveDir =
        QFileDialog::getExistingDirectory(nullptr, "选择保存提取文件的目录",
                                          defaultPath,                 // 默认打开的目录
                                          QFileDialog::ShowDirsOnly |  // 只显示目录（隐藏文件）
                                              QFileDialog::DontResolveSymlinks  // 不解析符号链接（可选，按需求决定）
        );

    if (saveDir.isEmpty())
    {
        return;
    }

    auto task_func = [this, saveDir](const std::function<void(float, float, std::string)> &update_progress,
                                     const std::function<bool()>                          &is_cancelled) {
        try
        {
            bool success = StegoCore::extractBinary(m_originalImage, saveDir, update_progress, is_cancelled);
            if (!success)
            {
                update_progress(0, 0, "提取失败");
                return false;
            }

            return success;
        }
        catch (const std::runtime_error &e)
        {
            update_progress(0, 0, "提取任务异常");
            return false;
        }
    };
    auto start_cb = [this]() {
        emit sigExtractStart();
    };
    auto process_cb = [this](const TaskProgress<float> &progress) {
        emit sigExtractProgress(progress.percentage, QString::fromStdString(progress.message));
    };
    TaskID taskId      = 0;
    auto   complete_cb = [this, &taskId](EM_TASK_STATE state, bool result) {
        emit sigExtractCompleted(state, result);
        removeTaskID(taskId);
    };
    taskId = AsyncTaskManager::get_instance().add_task<bool>(task_func, start_cb, process_cb, complete_cb);
    addTaskID(taskId);
}

void MainWindow::on_btnSaveImage_clicked()
{
    if (m_processedImage.isNull())
    {
        showMessage("无处理后的图片可保存", true);
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(this, "保存图片", "stego_image.png",
                                                    "PNG图片 (*.png);;JPG图片 (*.jpg *.jpeg);;BMP图片 (*.bmp)");
    if (filePath.isEmpty())
    {
        return;
    }
    auto task_func = [this, filePath](const std::function<void(float, float, std::string)> &update_progress,
                                      const std::function<bool()>                          &is_cancelled) {
        Q_UNUSED(update_progress)
        Q_UNUSED(is_cancelled)
        return m_processedImage.save(filePath);
    };
    auto start_cb = [this]() {
        emit sigImageSaveStart();
    };
    TaskID taskId      = 0;
    auto   complete_cb = [this, &taskId](EM_TASK_STATE state, bool result) {
        emit sigImageSaved(state, result);
        removeTaskID(taskId);
    };
    taskId = AsyncTaskManager::get_instance().add_task<bool>(task_func, start_cb, nullptr, complete_cb);
    addTaskID(taskId);
}

void MainWindow::on_btnCancelTask_clicked()
{
    cancelTasks();
}

void MainWindow::on_bitCountCombo_currentIndexChanged(int index)
{
    m_bitCount = index + 1;  // 索引0对应1位，索引1对应2位
    updateEmbedInfo();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::information(this, "关于", "隐写工具: V1.0.0");
    return;
}
