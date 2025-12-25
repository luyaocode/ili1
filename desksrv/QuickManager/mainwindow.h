#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTableView>
#include <QTreeWidget>
#include "toolmodel.h"
#include "commonmodel.h"
#include "tooldelegate.h"
#include "commondelegate.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    ToolModel   *getToolModel() const;
    CommonModel *getCommonModel() const;
signals:
    void sigOpenTerminal(const QString &path);
    void sigOpenDirectory(const QString &path);

private:
    void initMenubar();
    // 初始化快捷工具Tab页
    void initToolTabPage();
    // 初始化快捷目录Tab页
    void initCommonTabPage();
    // 加载配置
    void loadConfig();
private slots:
    void onRestoreDefault();
    void slotSaveConfig();

private:
    // 核心控件
    QTabWidget *m_mainTabWidget;    // 主TabWidget（左右分栏）
    QTableView *m_toolTableView;    // 快捷工具表格
    QTableView *m_commonTableView;  // 快捷目录树形控件

    ToolModel      *m_toolModel;
    CommonModel    *m_commonModel;
    ToolDelegate   *m_toolDelegate;
    CommonDelegate *m_commonDelegate;
};

#endif  // MAINWINDOW_H
