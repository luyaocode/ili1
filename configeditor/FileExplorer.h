#ifndef FILEEXPLORER_H
#define FILEEXPLORER_H

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>

class ButtonToolBar;
class FileExplorer : public QWidget
{
    Q_OBJECT

public:
    FileExplorer(QWidget *parent = nullptr);
    ~FileExplorer() = default;
    void openSourceDir();
signals:
    void fileSelected(const QString& filePath);
    // 扫描完成信号（用于跨线程更新UI）
    void scanFinished(QTreeWidgetItem *rootItem);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);
    // UI线程更新TreeWidget
    void onScanFinished(QTreeWidgetItem *rootItem);

private:
    void initUi();
    void buildFileTree(QTreeWidgetItem* parentItem, const QString& dirPath);
    bool isSupportedFile(const QFileInfo& fileInfo);
    // 同步接口
    void openDir(const QString& strDir);
    // 异步接口
    void openDirAsync(const QString& strDir);
    void openLastFile();
    void openFile(const QString& strFilePath);
    /**
     * @brief 查找QTreeWidget中UserRole存储路径为str的item，并设为选中
     * @param targetPath 目标文件路径（对应item的UserRole数据）
     * @return 找到返回true，未找到返回false
     */
    bool selectItemByFilePath(const QString& targetPath);
    QTreeWidgetItem* findItemByFilePath(QTreeWidgetItem* parentItem, const QString& targetPath);
private:
    QTreeWidget* m_fileTree = nullptr;
    ButtonToolBar* m_toolBar = nullptr;
    bool m_isScanning = false; // 避免重复扫描
};

#endif // FILEEXPLORER_H
