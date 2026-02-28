#ifndef MMAPFILE_H
#define MMAPFILE_H

#include <QString>
#include <QByteArray>

// 补充Linux系统头文件，解决MAP_FAILED未定义问题
#include <sys/mman.h>  // 包含MAP_FAILED、mmap、munmap等宏和函数声明
#include <fcntl.h>     // 包含open、O_RDWR等宏
#include <unistd.h>    // 包含close、ftruncate等函数声明

// Linux下MMAP文件封装类（C++11，Qt5.9.5兼容）
class MmapFile
{
public:
    // 构造/析构
    MmapFile();
    ~MmapFile();

    // 禁用拷贝（避免内存映射重复释放）
    MmapFile(const MmapFile &)            = delete;
    MmapFile &operator=(const MmapFile &) = delete;

    /**
     * @brief 初始化MMAP：创建/打开文件并映射到内存
     * @param file_path 映射的磁盘文件路径
     * @param map_size  映射的内存大小（字节），需提前规划足够空间
     * @return 成功返回true，失败返回false
     */
    bool init(const QString &file_path, qint64 map_size);

    /**
     * @brief 写入数据到MMAP内存（无write调用，直接写内存）
     * @param data   要写入的数据
     * @param offset 写入偏移量（从映射内存起始位置开始）
     * @return 成功返回true，失败返回false（如越界、未初始化）
     */
    bool writeData(const QByteArray &data, qint64 offset = 0);

    /**
     * @brief 从MMAP内存读取数据（无read调用，直接读内存）
     * @param offset 读取偏移量
     * @param size   读取数据大小
     * @return 读取到的字节数组，失败返回空
     */
    QByteArray readData(qint64 offset, qint64 size);

    /**
     * @brief 手动刷盘：强制将内存数据同步到磁盘（可选，系统也会异步刷）
     * @return 成功返回true，失败返回false
     */
    bool flush();

    /**
     * @brief 解除内存映射并释放资源
     */
    void unmap();

    /**
     * @brief 获取映射的内存大小
     */
    qint64 mapSize() const;

    /**
     * @brief 检查MMAP是否初始化成功
     */
    bool isInitialized() const;

private:
    QString m_file_path;  // 映射的磁盘文件路径
    void   *m_map_addr;   // 映射的内存起始地址（MMAP返回的地址）
    qint64  m_map_size;   // 映射的内存大小
    int     m_fd;         // Linux文件描述符
};

#endif  // MMAPFILE_H
