#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QFileDialog>
#include "mmapfile.h"

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // 原有槽函数
    void onWriteClicked();
    void onReadClicked();
    void onFlushClicked();
    // 新增槽函数：选择文件路径
    void onSelectPathClicked();
    // 新增槽函数：重新初始化MMAP（自定义路径/大小）
    void onReinitMmapClicked();

private:
    // MMAP核心实例
    MmapFile m_mmap;

    // UI控件（原有）
    QTextEdit* m_textEdit;

    // UI控件（新增：配置项）
    QLineEdit* m_filePathEdit;    // 文件路径输入框
    QLineEdit* m_fileNameEdit;    // 文件名输入框
    QSpinBox* m_mapSizeSpinBox;   // 映射大小输入框（单位：字节）
    QPushButton* m_selectPathBtn; // 选择路径按钮
    QPushButton* m_reinitMmapBtn; // 重新初始化MMAP按钮

    // 辅助函数：拼接完整文件路径
    QString getFullFilePath();
};

#endif // MAINWINDOW_H
