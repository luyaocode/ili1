#include "mmapfile.h"
#include <QDebug>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

MmapFile::MmapFile()
    : m_map_addr(MAP_FAILED)  // 初始化为MMAP失败标识
    , m_map_size(0)
    , m_fd(-1)
{
}

MmapFile::~MmapFile()
{
    // 析构时自动解除映射
    unmap();
}

bool MmapFile::init(const QString &file_path, qint64 map_size)
{
    // 校验参数
    if (map_size <= 0 || file_path.isEmpty())
    {
        qCritical() << "Invalid init params: size=" << map_size << ", path=" << file_path;
        return false;
    }

    // 1. 打开/创建文件（读写权限）
    m_fd = open(file_path.toLocal8Bit().data(), O_RDWR | O_CREAT, 0644);
    if (m_fd < 0)
    {
        qCritical() << "Open file failed: " << strerror(errno) << ", path=" << file_path;
        return false;
    }

    // 2. 设置文件大小（必须和映射大小一致）
    if (ftruncate(m_fd, map_size) < 0)
    {
        qCritical() << "Truncate file failed: " << strerror(errno);
        close(m_fd);
        m_fd = -1;
        return false;
    }

    // 3. 映射文件到进程内存
    m_map_addr = mmap(NULL,                    // 让系统自动选择映射地址
                      map_size,                // 映射大小
                      PROT_READ | PROT_WRITE,  // 内存读写权限
                      MAP_SHARED,              // 共享映射（修改会同步到磁盘）
                      m_fd,                    // 已打开的文件描述符
                      0                        // 文件偏移量（从开头映射）
    );

    // 校验映射结果
    if (m_map_addr == MAP_FAILED)
    {
        qCritical() << "MMAP failed: " << strerror(errno);
        close(m_fd);
        m_fd = -1;
        return false;
    }

    // 保存参数
    m_file_path = file_path;
    m_map_size  = map_size;

    qInfo() << "MMAP init success: path=" << file_path << ", size=" << map_size << ", mem addr=" << m_map_addr;
    return true;
}

bool MmapFile::writeData(const QByteArray &data, qint64 offset)
{
    // 校验状态和参数
    if (!isInitialized() || data.isEmpty() || offset < 0)
    {
        qCritical() << "Write failed: invalid state or params";
        return false;
    }

    // 校验是否越界
    if (offset + data.size() > m_map_size)
    {
        qCritical() << "Write out of bounds: offset=" << offset << ", data size=" << data.size()
                    << ", map size=" << m_map_size;
        return false;
    }

    // 直接拷贝数据到映射内存（无write调用，系统异步刷盘）
    memcpy(static_cast<char *>(m_map_addr) + offset, data.data(), data.size());
    qInfo() << "Write data success: size=" << data.size() << ", offset=" << offset;
    return true;
}

QByteArray MmapFile::readData(qint64 offset, qint64 size)
{
    // 校验状态和参数
    if (!isInitialized() || size <= 0 || offset < 0)
    {
        qCritical() << "Read failed: invalid state or params";
        return QByteArray();
    }

    // 校验是否越界
    if (offset + size > m_map_size)
    {
        qCritical() << "Read out of bounds: offset=" << offset << ", read size=" << size << ", map size=" << m_map_size;
        return QByteArray();
    }

    // 直接从映射内存拷贝数据
    QByteArray result(static_cast<char *>(m_map_addr) + offset, size);
    qInfo() << "Read data success: size=" << result.size() << ", offset=" << offset;
    return result;
}

bool MmapFile::flush()
{
    if (!isInitialized())
    {
        qCritical() << "Flush failed: MMAP not initialized";
        return false;
    }

    // 强制同步内存数据到磁盘（MS_SYNC：等待同步完成）
    int ret = msync(m_map_addr, m_map_size, MS_SYNC);
    if (ret != 0)
    {
        qCritical() << "Flush failed: " << strerror(errno);
        return false;
    }

    qInfo() << "Flush success: data synced to disk";
    return true;
}

void MmapFile::unmap()
{
    if (m_map_addr != MAP_FAILED)
    {
        // 解除内存映射
        munmap(m_map_addr, m_map_size);
        m_map_addr = MAP_FAILED;
    }

    if (m_fd >= 0)
    {
        // 关闭文件描述符
        close(m_fd);
        m_fd = -1;
    }

    m_map_size = 0;
    m_file_path.clear();

    qInfo() << "MMAP unmap completed";
}

qint64 MmapFile::mapSize() const
{
    return m_map_size;
}

bool MmapFile::isInitialized() const
{
    return (m_map_addr != MAP_FAILED) && (m_fd >= 0);
}
