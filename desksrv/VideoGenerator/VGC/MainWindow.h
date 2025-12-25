// MainWindow.h（更新后）
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QSpinBox>
#include <QLabel>
#include <QLineEdit>
#include <QFileDialog>
#include <mutex>
#include "unify/dynamic_library.hpp"
#include "plugin_interface.h"

struct Plugin
{
    PluginInfo     info;
    DynamicLibrary lib;
    Plugin(const PluginInfo& i,DynamicLibrary&& l):info(i),lib(std::move(l))
    {

    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 预览生成的帧（带滤镜）
    void onPreviewFrame();
    // 导出视频
    void onExportVideo();
    // 切换生成器插件
    void onSwitchGenerator(int selectedIdx);
    // 切换滤镜插件
    void onSwitchFilter(int selectedIdx);
    // 热更选中插件
    void onHotUpdatePlugin();
    // 热更插件列表
    void onHotUpdatePlugins();

public:
    // 生成单帧并应用滤镜
    QImage generateFilteredFrame(int frame_index);

private:
    // 扫描插件目录
    void scanPlugins(const std::string &plugin_dir, QListWidget *list_widget, std::vector<Plugin> &plugins);

private:
    std::vector<Plugin> m_plugins;  // 插件表
    // 生成器插件相关
    QListWidget      *m_generatorList;                  // 生成器插件列表
    GenerateFrameFunc m_currentGenerateFunc = nullptr;  // 帧生成函数
    EncodeVideoFunc   m_currentEncodeFunc   = nullptr;  // 视频编码函数

    // 滤镜插件相关
    QListWidget *m_filterList;
    FilterFunc   m_currentFilterFunc = nullptr;

    // 生成参数配置UI
    QSpinBox  *m_widthSpin;     // 宽度（像素）
    QSpinBox  *m_heightSpin;    // 高度（像素）
    QSpinBox  *m_fpsSpin;       // 帧率（fps）
    QSpinBox  *m_durationSpin;  // 时长（秒）
    QLineEdit *m_outputEdit;    // 输出文件路径
    QLabel    *m_previewLabel;  // 帧预览标签

    std::mutex m_pluginMutex;  // 线程安全锁
};

#endif  // MAINWINDOW_H
