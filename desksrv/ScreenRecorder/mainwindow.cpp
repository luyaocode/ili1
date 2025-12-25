#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QThread>
#include <QMouseEvent>
#include <QShortcut>
#include <QPainter>
#include <QDesktopServices>
#include <QScreen>
#include <QTimer>
#include <X11/keysym.h>
#include "tool.h"
#include "screenrecorder.h"
#include "keyblocker.h"

// 设置标签固定宽高（示例：400x300，可根据需要调整）
constexpr const int FIXED_WIDTH  = 400;
constexpr const int FIXED_HEIGHT = 300;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_pUi(std::shared_ptr<Ui::MainWindow>(new Ui::MainWindow)), m_isMinimized(false)
{
    m_pUi->setupUi(this);
    setWindowTitle("录屏工具");
    setFixedSize(size());
    // 窗口标志：无标题栏 + 始终置顶
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    moveToBottomLeft();
    // 适应分辨率变化
    connect(QApplication::desktop(), &QDesktopWidget::resized, this, &MainWindow::moveToBottomLeft);
    connect(QApplication::desktop(), &QDesktopWidget::screenCountChanged, this, &MainWindow::moveToBottomLeft);
    // 初始化全局快捷键（有问题，并非全局快捷键）
    //    initShortcuts();
    m_normalSize = size();
    m_normalPos  = pos();
    // 设置窗口背景为黑色
    setStyleSheet(getGlobalStyle());

    // 设置窗口透明度（0.0-1.0，值越小越透明）
    setWindowOpacity(1);
    m_normalWindowOpacity = windowOpacity();  // 保存正常状态的透明度

    m_pScreenRecorder = std::shared_ptr<ScreenRecorder>(new ScreenRecorder);
    m_pRecordThread   = new QThread(this);
    m_pRecordThread->setObjectName("thread_record");
    m_pScreenRecorder->moveToThread(m_pRecordThread);
    connect(this, &MainWindow::sigStartRecord, m_pScreenRecorder.get(), &ScreenRecorder::startRecording);
    connect(this, &MainWindow::sigStopRecord, m_pScreenRecorder.get(), &ScreenRecorder::stopRecording);
    // 确保线程可以安全退出
    connect(m_pRecordThread, &QThread::finished, m_pScreenRecorder.get(), &QObject::deleteLater);
    connect(m_pRecordThread, &QThread::finished, m_pRecordThread, &QThread::deleteLater);
    connect(m_pScreenRecorder.get(), &ScreenRecorder::recordingStateChanged, this, [this](bool isRecording) {
        m_isMinimized = !isRecording;
        toggleWindowState();
    });
    connect(m_pScreenRecorder.get(), &ScreenRecorder::frameAvailable, this, [this](const cv::Mat &frame) {
        // 将cv::Mat转换为QImage
        QImage image = cvMatToQImage(frame);
        if (image.isNull())
            return;  // 防止空图像导致问题

        // 将QImage转换为QPixmap并按QLabel大小自动缩放
        QPixmap pixmap = QPixmap::fromImage(image);

        // 缩放图像以适应QLabel，保持比例并填充整个空间
        QPixmap scaledPixmap = pixmap.scaled(m_pUi->label_screen_record->size(),  // 目标大小（QLabel的当前大小）
                                             Qt::KeepAspectRatio,                 // 保持宽高比
                                             Qt::SmoothTransformation             // 平滑缩放，画质更好
        );

        // 设置缩放后的图像到QLabel
        m_pUi->label_screen_record->setPixmap(scaledPixmap);
    });
    connect(m_pScreenRecorder.get(), &ScreenRecorder::sigFileSaved, this, [this](const QString &file) {
        QMessageBox msgBox(QMessageBox::Information, "录屏", file + " saved!", QMessageBox::NoButton, nullptr);
        // 添加自定义按钮
        QPushButton *okBtn = msgBox.addButton(tr("确定"), QMessageBox::AcceptRole);
        (void)okBtn;
        QPushButton *openBtn = msgBox.addButton(tr("打开文件"), QMessageBox::ActionRole);
        msgBox.exec();
        // 判断用户点击了哪个按钮
        if (msgBox.clickedButton() == openBtn)
        {
            // 处理打开文件的逻辑，例如使用QDesktopServices打开文件所在位置
            if (!openFileDir(file))
            {
                // 如果打开失败，显示错误信息
                QFileInfo fileInfo(file);
                QString   dir = fileInfo.absoluteDir().absolutePath();
                QMessageBox::warning(nullptr, "错误", "无法打开目录：" + dir);
            }
        }
    });

    // 当主窗口销毁时停止线程
    connect(this, &MainWindow::destroyed, this, [this]() {
        m_pRecordThread->quit();
        m_pRecordThread->wait();
    });
    m_pRecordThread->start();

    // F1 监听
    m_F1blocker.reset(new KeyBlocker({KeyWithModifier(XK_F1)}));
    connect(m_F1blocker.data(), &KeyBlocker::sigBlocked, this, &MainWindow::toggleWindowState);
    m_F1blocker->start();
}

MainWindow::~MainWindow()
{
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    // 只有在小球状态下点击才触发停止录制
    if (m_isMinimized && event->button() == Qt::LeftButton)
    {
        // 触发停止录制
        emit sigStopRecord();
        event->accept();  // 接受事件，防止进一步传播
        return;
    }
    event->ignore();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    // 空实现，阻止窗口移动
    event->ignore();
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
    // 如果处于小球状态，绘制红色背景和REC文字
    if (m_isMinimized)
    {
        QPainter painter(this);
        // 设置抗锯齿，使圆形边缘更平滑
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 绘制红色圆形背景
        painter.setBrush(QBrush(Qt::red));
        painter.setPen(Qt::NoPen);    // 去掉边框
        painter.drawEllipse(rect());  // 绘制与窗口大小相同的圆形

        // 绘制白色REC文字
        painter.setPen(QPen(Qt::white));
        QFont font = painter.font();
        font.setBold(true);
        font.setPointSize(12);
        painter.setFont(font);

        // 文字居中显示
        painter.drawText(rect(), Qt::AlignCenter, "stop");
    }
}

void MainWindow::on_startRecordBtn_clicked()
{
    emit sigStartRecord();
}

void MainWindow::on_stopRecordBtn_clicked()
{
    emit sigStopRecord();
}

void MainWindow::on_btn_open_save_dir_clicked()
{
    // 获取应用目录
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);

    // 检查桌面路径是否有效
    if (dataDir.isEmpty())
    {
        QMessageBox::warning(nullptr, "错误", "无法获取目录路径");
        return;
    }

    // 使用系统默认的文件管理器打开目录
    QUrl url = QUrl::fromLocalFile(dataDir);
    if (!QDesktopServices::openUrl(url))
    {
        // 如果打开失败，显示错误信息
        QMessageBox::warning(nullptr, "错误", "无法打开目录：" + dataDir);
    }
}

void MainWindow::on_btn_screenshoot_clicked()
{
    hide();
    QTimer::singleShot(200, [this]() {
        m_pScreenRecorder->screenShoot();
        show();
    });
}

void MainWindow::on_btn_exit_clicked()
{
    close();
}

void MainWindow::moveToBottomLeft()
{
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen)
        return;

    QRect screenRect = screen->geometry();  // 使用整个屏幕区域，包括任务栏

    // 计算位置
    int x = screenRect.left();
    int y = screenRect.bottom() - this->height();

    // 对于某些窗口管理器，可能需要使用窗口标志
    this->move(x, y);

    // 立即刷新窗口位置
    this->show();
    this->activateWindow();
}

void MainWindow::toggleWindowState()
{
    if (m_isMinimized)
    {
        // 恢复到正常窗口状态
        setFixedSize(m_normalSize);
        move(m_normalPos);
        m_pUi->centralWidget->show();  // 显示主界面
    }
    else
    {
        // 缩小为红色小球状态
        m_normalSize = size();  // 保存当前大小
        m_normalPos  = pos();   // 保存当前位置

        // 设置小球大小（50x50）
        setFixedSize(50, 50);
        // 保持在左下角
        moveToBottomLeft();
        // 隐藏主界面内容
        m_pUi->centralWidget->hide();
    }

    m_isMinimized = !m_isMinimized;
    update();  // 触发重绘
}
