#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSplitter>
#include <QStackedWidget>
#include <QTableWidget>
#include "FileExplorer.h"
#include "XmlViewer.h"
#include "TextViewer.h"
#include "AttrViewer.h"
#include "buttontoolbar.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow() = default;
private slots:
    void onFileSelected(const QString& filePath);

private:
    void initUi();
    void initConnections();
private:
    QSplitter* m_splitter= nullptr;
    FileExplorer* m_fileExplorer= nullptr;
    QStackedWidget* m_contentStack= nullptr;
    AttrViewer* m_attrViewer= nullptr;
    XmlViewer* m_xmlViewer= nullptr;
    TextViewer* m_textViewer= nullptr;
    TopButtonToolBar* m_toolBar = nullptr;
    bool m_initialized = false;
};

#endif // MAINWINDOW_H
