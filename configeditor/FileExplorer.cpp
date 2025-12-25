#include "FileExplorer.h"
#include <QDebug>
#include <QtConcurrent>
#include <QMessageBox>
#include "ConfigManager.h"
#include "buttontoolbar.h"
#include "Tool.h"

FileExplorer::FileExplorer(QWidget *parent)
    : QWidget(parent)
{
    initUi();
}

void FileExplorer::openLastFile()
{
    QString lastFilePath;
    QString lastLoadTime;
    bool loadSuccess;
    pConfigMgr->readLastOpenFile(lastFilePath,lastLoadTime,loadSuccess);
    selectItemByFilePath(lastFilePath);
    openFile(lastFilePath);
}

void FileExplorer::initUi()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // 文件树（默认展开所有节点）
    m_fileTree = new QTreeWidget(this);
    m_fileTree->setColumnCount(1);
    m_fileTree->setHeaderHidden(true);
    m_fileTree->setExpandsOnDoubleClick(true);

    // 工具栏
    m_toolBar = new ButtonToolBar(this);
    m_toolBar->initControls();
    m_toolBar->addButton("",[this](){
        m_fileTree->expandAll();
    },"全部展开",getIcon(QStyle::SP_ArrowRight));
    m_toolBar->addButton("",[this](){
        m_fileTree->collapseAll();
    },"全部折叠",getIcon(QStyle::SP_ArrowLeft));

    layout->addWidget(m_toolBar);
    layout->addWidget(m_fileTree);

    // 连接信号
    connect(m_fileTree, &QTreeWidget::itemClicked, this, &FileExplorer::onItemClicked);

    // 打开默认目录
    QString lastFilePath;
    QString lastLoadTime;
    bool loadSuccess;
    pConfigMgr->readLastOpenDir(lastFilePath,lastLoadTime,loadSuccess);
    openDirAsync(lastFilePath);
}

void FileExplorer::openSourceDir()
{
    if(m_isScanning)
    {
        QMessageBox::information(this,"提示","请等待当前操作完成...");
        return;
    }
    QString dirPath = QFileDialog::getExistingDirectory(this, "选择目录");
    if (dirPath.isEmpty()) return;
    openDir(dirPath);
    pConfigMgr->writeLastOpenDir(dirPath,true);
}

void FileExplorer::buildFileTree(QTreeWidgetItem* parentItem, const QString& dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) return;

    // 获取所有文件和目录（排序：目录在前，文件在后）
    // 正确分离过滤标志和排序标志，分别传递到对应参数
    QFileInfoList fileList = dir.entryInfoList(
        QStringList(), // 无文件过滤（显示所有文件）
        QDir::AllEntries | QDir::NoDotAndDotDot, // 过滤标志：所有条目 + 排除.和..
        QDir::Name | QDir::DirsFirst // 排序标志：按名称排序 + 目录在前
    );

    for (const QFileInfo& fileInfo : fileList)
    {
        QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
        item->setText(0, fileInfo.fileName());
        item->setData(0, Qt::UserRole, fileInfo.filePath());

        // 目录：递归构建子节点，设置图标
        if (fileInfo.isDir())
        {
            item->setIcon(0, QIcon::fromTheme("folder"));
            buildFileTree(item, fileInfo.filePath());
        }
        // 文件：设置图标，过滤不支持的文件（可选）
        else
        {
            item->setIcon(0, QIcon::fromTheme("text-plain"));
            if (!isSupportedFile(fileInfo))
            {
                item->setDisabled(true); // 禁用不支持的文件
                item->setForeground(0, Qt::gray); // 灰色显示
            }
        }
    }
}

bool FileExplorer::isSupportedFile(const QFileInfo& fileInfo)
{
    // 支持XML和所有文本类文件（可根据需求扩展）
    QString suffix = fileInfo.suffix().toLower();
    QStringList supportedSuffixes = {"xml", "txt", "log", "cpp", "h", "pro", "qrc"};
    return supportedSuffixes.contains(suffix) || fileInfo.isReadable();
}

void FileExplorer::openDir(const QString &strDir)
{
    if(strDir.isEmpty()) return;
    // 清空现有树结构
    m_fileTree->clear();

    // 构建根节点
    QTreeWidgetItem* rootItem = new QTreeWidgetItem(m_fileTree);
    rootItem->setText(0, QDir(strDir).dirName());
    rootItem->setData(0, Qt::UserRole, strDir);

    // 递归构建文件树
    buildFileTree(rootItem, strDir);

    // 连接信号槽（确保在UI线程执行更新）
    connect(this, &FileExplorer::scanFinished,
           this, &FileExplorer::onScanFinished,
           Qt::QueuedConnection);
    emit scanFinished(rootItem);
}

void FileExplorer::openDirAsync(const QString &strDir)
{
    if (strDir.isEmpty() || m_isScanning) return;

    m_isScanning = true;
    m_fileTree->clear(); // 清空现有内容（UI线程轻量操作）

    QDir rootDir(strDir);
    if (!rootDir.exists()) {
       m_isScanning = false;
       return;
    }

    // 连接信号槽（确保在UI线程执行更新）
    connect(this, &FileExplorer::scanFinished,
           this, &FileExplorer::onScanFinished,
           Qt::QueuedConnection);
    // 异步执行扫描（自动创建临时线程，执行完自动销毁）
    QtConcurrent::run([this,strDir]() {
        // 构建根节点
        QTreeWidgetItem* rootItem = new QTreeWidgetItem;
        rootItem->setText(0, QDir(strDir).dirName());
        rootItem->setData(0, Qt::UserRole, strDir);
        // 递归构建文件树
        buildFileTree(rootItem, strDir);
        // 发送信号，将结果传递到UI线程
        emit scanFinished(rootItem);
    });
}

void FileExplorer::openFile(const QString &strFilePath)
{
    QFileInfo fileInfo(strFilePath);

    // 只处理文件，不处理目录
    if (fileInfo.isFile() && isSupportedFile(fileInfo))
    {
        emit fileSelected(strFilePath);
    }
}

// 核心：查找并选中目标item
bool FileExplorer::selectItemByFilePath(const QString& targetPath)
{
    if (m_fileTree == nullptr || targetPath.isEmpty()) {
        qWarning() << "[XmlViewer] 无效参数：m_fileTree为空或目标路径为空";
        return false;
    }

    // 1. 递归遍历所有节点（从根节点开始）
    QTreeWidgetItem* targetItem = findItemByFilePath(nullptr, targetPath);

    if (targetItem != nullptr) {
        // 2. 找到item后设置为选中
        m_fileTree->setCurrentItem(targetItem); // 设置当前选中项
        targetItem->setSelected(true);          // 强制选中状态

        // （可选）展开父节点，确保选中项可见
        QTreeWidgetItem* parentItem = targetItem->parent();
        while (parentItem != nullptr) {
            parentItem->setExpanded(true);
            parentItem = parentItem->parent();
        }

        qInfo() << "[XmlViewer] 成功选中路径对应的item：" << targetPath;
        return true;
    } else {
        qWarning() << "[XmlViewer] 未找到路径对应的item：" << targetPath;
        return false;
    }
}

// 递归遍历辅助函数
QTreeWidgetItem* FileExplorer::findItemByFilePath(QTreeWidgetItem* parentItem, const QString& targetPath)
{
    // 首次调用：parentItem为nullptr，遍历顶层节点
    QList<QTreeWidgetItem*> itemList;
    if (parentItem == nullptr) {
        itemList = m_fileTree->invisibleRootItem()->takeChildren(); // 取出顶层节点
        m_fileTree->invisibleRootItem()->addChildren(itemList);     // 重新添加（不改变结构）
    } else {
        itemList = parentItem->takeChildren(); // 取出子节点
        parentItem->addChildren(itemList);     // 重新添加（不改变结构）
    }

    // 遍历当前层级节点
    for (QTreeWidgetItem* item : itemList) {
        // 读取item的Qt::UserRole数据（存储的文件路径）
        QString itemFilePath = item->data(0, Qt::UserRole).toString();

        // 路径匹配（注意：区分大小写，若需不区分可使用QString::compare并设置Qt::CaseInsensitive）
        if (itemFilePath == targetPath) {
            return item; // 找到目标item，返回
        }

        // 递归遍历子节点
        QTreeWidgetItem* childResult = findItemByFilePath(item, targetPath);
        if (childResult != nullptr) {
            return childResult; // 子节点中找到，向上返回
        }
    }

    return nullptr; // 未找到
}

void FileExplorer::onItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    QString filePath = item->data(0, Qt::UserRole).toString();
    if(!filePath.isEmpty())
    {
        openFile(filePath);
        pConfigMgr->writeLastOpenFile(filePath,true);
    }
}

void FileExplorer::onScanFinished(QTreeWidgetItem *rootItem)
{
    if (rootItem) {
        m_fileTree->addTopLevelItem(rootItem); // 一次性添加所有节点
        m_fileTree->expandAll();
    }
    m_isScanning = false;
    openLastFile();
}
