#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QImage>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QLabel>
#include <QMutex>
#include <QMutexLocker>

#include "StegoCore.h"
#include "unify/asynctaskmanager.hpp"
#include "commontool/widgetfilter.hpp"

USING_NAMESAPCE(unify)

namespace Ui
{
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void sigImageCreated();
    // 异步任务信号
    void sigEmbedProgress(float pct, const QString &msg = "");
    void sigEmbedStart();
    void sigEmbedCompleted(unify::EM_TASK_STATE state, bool result);
    void sigExtractProgress(float pct, const QString &msg = "");
    void sigExtractStart();
    void sigExtractCompleted(unify::EM_TASK_STATE state, bool result);
    void sigImageSaveStart();
    void sigImageSaved(unify::EM_TASK_STATE state, bool result);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void on_btnOpenImage_clicked();
    void on_btnGenImage_clicked();
    void on_btnEmbed_clicked();
    void on_btnExtract_clicked();
    void on_btnEmbedBinary_clicked();
    void on_btnExtractBinary_clicked();
    void on_btnSaveImage_clicked();
    void on_btnCancelTask_clicked();
    void on_bitCountCombo_currentIndexChanged(int index);
    void on_actionAbout_triggered();
    void updateEmbedInfo();
    // 异步任务
    void slotImageCreated();
    void slotEmbedStart();
    void slotEmbedProgress(float pct, const QString &msg = "");
    void slotEmbedCompleted(unify::EM_TASK_STATE state, bool result);
    void slotExtractProgress(float pct, const QString &msg = "");
    void slotExtractStart();
    void slotExtractCompleted(unify::EM_TASK_STATE state, bool result);
    void slotImageSaveStart();
    void slotImageSaved(unify::EM_TASK_STATE state, bool result);

private:
    // 初始化
    void initGenImageSizeComboBox();
    // 保存选中的图片宽高
    void saveSelectedImageSize(int index);

    void loadImage(const QString &filePath);
    void loadImage(const QImage &image);
    void showImagePreview(QImage &image, QLabel *label);
    void showMessage(const QString &text, bool isError = false);
    void addTaskID(TaskID id);
    void removeTaskID(TaskID id);
    void cancelTasks();

private:
    Ui::MainWindow *ui = nullptr;
    QImage          m_originalImage;
    QImage          m_processedImage;
    QSize           m_genImgSize;
    int             m_bitCount = 1;  // 默认1位LSB嵌入
    WidgetFilter    m_lastFilter;    // 上次界面禁用条件
    QMutex          m_mutex;         // 互斥锁
    QList<TaskID>   m_tasks;         // 当前任务列表
};

#endif  // MAINWINDOW_H
