#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include <opencv2/opencv.hpp>
#include <mutex>

struct KeyWithModifier;
class KeyBlocker;


namespace Ui
{
    class MainWindow;
}
class QMouseEvent;
class QKeyEvent;
class QShortcut;
class ScreenRecorder;
class QThread;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;  // 用于绘制小球状态的REC文字
signals:
    void sigStartRecord(const QString &outputPath = QString());
    void sigStopRecord();
private slots:
    void on_startRecordBtn_clicked();
    void on_stopRecordBtn_clicked();
    void on_btn_open_save_dir_clicked();
    void on_btn_screenshoot_clicked();
    void on_btn_exit_clicked();

private:
    void moveToBottomLeft();
    void toggleWindowState();  // 切换窗口状态
private:
    std::shared_ptr<Ui::MainWindow> m_pUi;
    bool                            m_isMinimized;          // 标记是否处于小球状态
    QSize                           m_normalSize;           // 正常窗口大小
    QPoint                          m_normalPos;            // 正常窗口位置
    qreal                           m_normalWindowOpacity;  // 保存正常状态的透明度
    cv::Mat                         m_OrgImg;               // 原始图像
    cv::Mat                         m_ProcessedImg;         // 处理后的图像
    std::shared_ptr<ScreenRecorder> m_pScreenRecorder;
    QThread                        *m_pRecordThread = nullptr;

    QScopedPointer<KeyBlocker> m_F1blocker;  // F1 按键拦截
};

#endif  // MAINWINDOW_H
